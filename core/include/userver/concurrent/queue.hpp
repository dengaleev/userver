#pragma once

#include <atomic>
#include <memory>

#include <moodycamel/concurrentqueue.h>

#include <userver/concurrent/queue_helpers.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/atomic.hpp>

namespace concurrent {

/// Queue with single and multi producer/consumer options
///
/// @see @ref md_en_userver_synchronization
template <typename T, bool MultipleProducer, bool MultipleConsumer>
class GenericQueue final
    : public std::enable_shared_from_this<
          GenericQueue<T, MultipleProducer, MultipleConsumer>> {
 private:
  class EmplaceEnabler {};
  using ProducerToken =
      std::conditional_t<MultipleProducer, moodycamel::ProducerToken,
                         impl::NoToken>;
  using ConsumerToken =
      std::conditional_t<MultipleProducer, moodycamel::ConsumerToken,
                         impl::NoToken>;

  using SingleProducerToken =
      std::conditional_t<!MultipleProducer, moodycamel::ProducerToken,
                         impl::NoToken>;

 public:
  using ValueType = T;

  using Producer = impl::Producer<GenericQueue>;
  using Consumer = impl::Consumer<GenericQueue>;

  friend class impl::Producer<GenericQueue>;
  friend class impl::Consumer<GenericQueue>;

  static constexpr std::size_t kUnbounded =
      std::numeric_limits<std::size_t>::max() / 2;

  /// For internal use only
  explicit GenericQueue(std::size_t max_size, EmplaceEnabler /*unused*/)
      : queue_(),
        capacity_(kUnbounded),
        single_producer_token_(queue_),
        producer_side_(*this, kUnbounded),
        consumer_side_(*this, kUnbounded) {
    max_size = std::min(max_size, kUnbounded);
    consumer_side_.DecreaseSize(kUnbounded);
    SetSoftMaxSize(max_size);
  }

  ~GenericQueue() {
    UASSERT(consumers_count_ == kCreatedAndDead || !consumers_count_);
    UASSERT(producers_count_ == kCreatedAndDead || !producers_count_);

    // Clear remaining items in queue
    T value;
    ConsumerToken token{queue_};
    while (consumer_side_.PopNoblock(token, value)) {
    }

    // Return value of semaphores to start state.
    if (producers_count_ == kCreatedAndDead) {
      consumer_side_.DecreaseSize(kSemaphoreUnlockValue);
    }
    if (consumers_count_ == kCreatedAndDead) {
      producer_side_.DecreaseCapacity(kSemaphoreUnlockValue);
    }

    SetSoftMaxSize(kUnbounded);
    consumer_side_.IncreaseSize(kUnbounded);
  }

  GenericQueue(GenericQueue&&) = delete;
  GenericQueue(const GenericQueue&) = delete;
  GenericQueue& operator=(GenericQueue&&) = delete;
  GenericQueue& operator=(const GenericQueue&) = delete;

  /// Create a new queue
  static std::shared_ptr<GenericQueue> Create(
      std::size_t max_size = kUnbounded) {
    return std::make_shared<GenericQueue>(max_size, EmplaceEnabler{});
  }

  /// Producer may outlive the queue and the consumer.
  Producer GetProducer() {
    std::size_t old_producers_count{};
    utils::AtomicUpdate(producers_count_, [&](auto old_value) {
      old_producers_count = old_value;
      return old_value == kCreatedAndDead ? 1 : old_value + 1;
    });

    if (old_producers_count == kCreatedAndDead) {
      consumer_side_.DecreaseSize(kSemaphoreUnlockValue);
    }
    UASSERT(MultipleProducer || old_producers_count != 1);

    return Producer(this->shared_from_this(), EmplaceEnabler{});
  }

  /// Consumer may outlive the queue and the producer.
  Consumer GetConsumer() {
    std::size_t old_consumers_count{};
    utils::AtomicUpdate(consumers_count_, [&](auto old_value) {
      old_consumers_count = old_value;
      return old_value == kCreatedAndDead ? 1 : old_value + 1;
    });

    if (old_consumers_count == kCreatedAndDead) {
      producer_side_.DecreaseCapacity(kSemaphoreUnlockValue);
    }
    UASSERT(MultipleConsumer || old_consumers_count != 1);
    return Consumer(this->shared_from_this(), EmplaceEnabler{});
  }

  /// @brief Sets the limit on the queue size, pushes over this limit will block
  /// @note This is a soft limit and may be slightly overrun under load.
  /// @note If the current queue size is greater than new max size, this call
  /// will block until size becomes less than the new max size
  void SetSoftMaxSize(std::size_t max_size) {
    max_size = std::min(max_size, kUnbounded);
    const auto old_capacity = capacity_.exchange(max_size);
    if (max_size > old_capacity) {
      producer_side_.IncreaseCapacity(max_size - old_capacity);
    } else if (max_size < old_capacity) {
      producer_side_.DecreaseCapacity(old_capacity - max_size);
    }
  }

  /// @brief Gets the limit on the queue size
  std::size_t GetSoftMaxSize() const { return capacity_; }

  /// @brief Gets the approximate size of queue
  std::size_t GetSizeApproximate() const { return consumer_side_.GetSize(); }

 private:
  /// Proxy-class makes synchronization of Push operations in multi or single
  /// producer cases
  template <bool MultiProducer>
  class ProducerSide {};

  /// Proxy-class makes synchronization of Pop operations in multi or single
  /// consumer cases
  template <bool MultiConsumer>
  class ConsumerSide {};

  /// Single producer PushSync implementation
  template <>
  class ProducerSide<false> {
   public:
    ProducerSide(GenericQueue& queue, std::size_t max_size)
        : queue_(queue), remaining_capacity_(max_size) {}

    /// Blocks if there is a consumer to Pop the current value and task
    /// shouldn't cancel and queue if full
    [[nodiscard]] bool Push(ProducerToken& token, T&& value,
                            engine::Deadline deadline) {
      if (DoPush(token, std::move(value))) {
        return true;
      }

      return nonfull_event_.WaitForEventUntil(deadline) &&
             DoPush(token, std::move(value));
    }

    [[nodiscard]] bool PushNoblock(ProducerToken& token, T&& value) {
      return DoPush(token, std::move(value));
    }

    void OnElementPopped() {
      ++remaining_capacity_;
      nonfull_event_.Send();
    }

    void DecreaseCapacity(std::size_t count) { remaining_capacity_ -= count; }

    void IncreaseCapacity(std::size_t count) {
      remaining_capacity_ += count;
      nonfull_event_.Send();
    }

   private:
    [[nodiscard]] bool DoPush(ProducerToken& token, T&& value) {
      if (queue_.NoMoreConsumers() || !remaining_capacity_) {
        return false;
      }

      --remaining_capacity_;
      queue_.DoPush(token, std::move(value));
      nonfull_event_.Reset();
      return true;
    }

    GenericQueue& queue_;
    engine::SingleConsumerEvent nonfull_event_;
    std::atomic<std::size_t> remaining_capacity_;
  };

  /// Multi producer PushSync implementation
  template <>
  class ProducerSide<true> {
   public:
    ProducerSide(GenericQueue& queue, std::size_t max_size)
        : queue_(queue), remaining_capacity_(max_size) {}

    /// Blocks if there is a consumer to Pop the current value and task
    /// shouldn't cancel and queue if full
    [[nodiscard]] bool Push(ProducerToken& token, T&& value,
                            engine::Deadline deadline) {
      return !engine::current_task::ShouldCancel() &&
             remaining_capacity_.try_lock_shared_until(deadline) &&
             DoPush(token, std::move(value));
    }

    [[nodiscard]] bool PushNoblock(ProducerToken& token, T&& value) {
      return remaining_capacity_.try_lock_shared() &&
             DoPush(token, std::move(value));
    }

    void OnElementPopped() { remaining_capacity_.unlock_shared(); }

    void DecreaseCapacity(std::size_t count) {
      remaining_capacity_.try_lock_shared_until_count({}, count);
    }

    void IncreaseCapacity(std::size_t count) {
      remaining_capacity_.unlock_shared_count(count);
    }

   private:
    [[nodiscard]] bool DoPush(ProducerToken& token, T&& value) {
      if (queue_.NoMoreConsumers()) {
        remaining_capacity_.unlock_shared();
        return false;
      }

      queue_.DoPush(token, std::move(value));
      return true;
    }

    GenericQueue& queue_;
    engine::Semaphore remaining_capacity_;
  };

  /// Single consumer ConsumerSide implementation
  template <>
  class ConsumerSide<false> {
   public:
    ConsumerSide(GenericQueue& queue, std::size_t max_size)
        : queue_(queue), size_(max_size) {}

    /// Blocks only if queue is empty
    [[nodiscard]] bool Pop(ConsumerToken& token, T& value,
                           engine::Deadline deadline) {
      if (DoPop(token, value)) {
        return true;
      }

      if (queue_.NoMoreProducers() ||
          nonempty_event_.WaitForEventUntil(deadline)) {
        return DoPop(token, value);
      }

      return false;
    }

    [[nodiscard]] bool PopNoblock(ConsumerToken& token, T& value) {
      return DoPop(token, value);
    }

    void OnElementPushed() {
      ++size_;
      nonempty_event_.Send();
    }

    void DecreaseSize(std::size_t count) { size_ -= count; }

    void IncreaseSize(std::size_t count) {
      size_ += count;
      nonempty_event_.Send();
    }

    std::size_t GetSize() const { return size_; }

   private:
    [[nodiscard]] bool DoPop(ConsumerToken& token, T& value) {
      if (queue_.DoPop(token, value)) {
        --size_;
        nonempty_event_.Reset();
        return true;
      }
      return false;
    }

    GenericQueue& queue_;
    engine::SingleConsumerEvent nonempty_event_;
    std::atomic<std::size_t> size_;
  };

  /// Multi consumer ConsumerSide implementation
  template <>
  class ConsumerSide<true> {
   public:
    ConsumerSide(GenericQueue& queue, std::size_t max_size)
        : queue_(queue), size_(max_size) {}

    /// Blocks only if queue is empty
    [[nodiscard]] bool Pop(ConsumerToken& token, T& value,
                           engine::Deadline deadline) {
      return size_.try_lock_shared_until(deadline) && DoPop(token, value);
    }

    [[nodiscard]] bool PopNoblock(ConsumerToken& token, T& value) {
      return size_.try_lock_shared() && DoPop(token, value);
    }

    void OnElementPushed() { size_.unlock_shared(); }

    void DecreaseSize(std::size_t count) {
      size_.try_lock_shared_until_count({}, count);
    }

    void IncreaseSize(std::size_t count) { size_.unlock_shared_count(count); }

    std::size_t GetSize() const { return size_.RemainingApprox(); }

   private:
    [[nodiscard]] bool DoPop(ConsumerToken& token, T& value) {
      if (!queue_.DoPop(token, value)) {
        size_.unlock_shared();
        return false;
      }
      return true;
    }

    GenericQueue& queue_;
    engine::Semaphore size_;
  };

  [[nodiscard]] bool Push(ProducerToken& token, T&& value,
                          engine::Deadline deadline) {
    return producer_side_.Push(token, std::move(value), deadline);
  }

  [[nodiscard]] bool PushNoblock(ProducerToken& token, T&& value) {
    return producer_side_.PushNoblock(token, std::move(value));
  }

  [[nodiscard]] bool Pop(ConsumerToken& token, T& value,
                         engine::Deadline deadline) {
    return consumer_side_.Pop(token, value, deadline);
  }

  [[nodiscard]] bool PopNoblock(ConsumerToken& token, T& value) {
    return consumer_side_.PopNoblock(token, value);
  }

  void MarkConsumerIsDead() {
    const auto new_consumers_count =
        utils::AtomicUpdate(consumers_count_, [](auto old_value) {
          return old_value == 1 ? kCreatedAndDead : old_value - 1;
        });
    if (new_consumers_count == kCreatedAndDead) {
      producer_side_.IncreaseCapacity(kSemaphoreUnlockValue);
    }
  }

  void MarkProducerIsDead() {
    const auto new_producers_count =
        utils::AtomicUpdate(producers_count_, [](auto old_value) {
          return old_value == 1 ? kCreatedAndDead : old_value - 1;
        });
    if (new_producers_count == kCreatedAndDead) {
      consumer_side_.IncreaseSize(kSemaphoreUnlockValue);
    }
  }

  bool NoMoreConsumers() const { return consumers_count_ == kCreatedAndDead; }

  bool NoMoreProducers() const { return producers_count_ == kCreatedAndDead; }

  void DoPush(ProducerToken& token, T&& value) {
    if constexpr (MultipleProducer) {
      queue_.enqueue(token, std::move(value));
    } else {
      // If producer is single substitute his token
      queue_.enqueue(single_producer_token_, std::move(value));
    }

    consumer_side_.OnElementPushed();
  }

  [[nodiscard]] bool DoPop(ConsumerToken& token, T& value) {
    bool success = false;
    if constexpr (MultipleProducer) {
      success = queue_.try_dequeue(token, value);
    } else {
      // Substitute with our single producer token
      success = queue_.try_dequeue_from_producer(single_producer_token_, value);
    }

    if (success) {
      producer_side_.OnElementPopped();
      return true;
    }

    return false;
  }

 private:
  moodycamel::ConcurrentQueue<T> queue_{1};
  std::atomic<std::size_t> consumers_count_{0};
  std::atomic<std::size_t> producers_count_{0};
  std::atomic<std::size_t> capacity_;

  SingleProducerToken single_producer_token_;

  ProducerSide<MultipleProducer> producer_side_;
  ConsumerSide<MultipleConsumer> consumer_side_;

  static constexpr std::size_t kCreatedAndDead =
      std::numeric_limits<std::size_t>::max();
  static constexpr std::size_t kSemaphoreUnlockValue =
      std::numeric_limits<std::size_t>::max() / 2;
};

/// @ingroup userver_concurrency
template <typename T>
using NonFifoMpmcQueue = GenericQueue<T, true, true>;

/// @ingroup userver_concurrency
template <typename T>
using NonFifoMpscQueue = GenericQueue<T, true, false>;

/// @ingroup userver_concurrency
template <typename T>
using NonFifoSpmcQueue = GenericQueue<T, false, true>;

/// @ingroup userver_concurrency
template <typename T>
using NonFifoSpscQueue = GenericQueue<T, false, false>;

}  // namespace concurrent
