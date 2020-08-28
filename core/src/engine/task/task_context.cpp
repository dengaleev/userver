#include "task_context.hpp"

#include <exception>

#include <boost/stacktrace.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/timer.hpp>
#include <engine/exception.hpp>
#include <engine/task/cancel.hpp>
#include <utils/assert.hpp>

#include <engine/task/coro_unwinder.hpp>
#include <engine/task/cxxabi_eh_globals.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/wait_list_light.hpp>

namespace engine {
namespace current_task {
namespace {

thread_local impl::TaskContext* current_task_context_ptr = nullptr;

void SetCurrentTaskContext(impl::TaskContext* context) {
  UASSERT(!current_task_context_ptr || !context);
  current_task_context_ptr = context;
}

// This MUST be a separate function! Putting the body of this function into
// GetCurrentTaskContext() clobbers too many registers and compiler decides to
// use stack memory in GetCurrentTaskContext(). This leads to slowdown
// of GetCurrentTaskContext(). In particular Mutex::lock() slows down on ~25%.
[[noreturn]] void ReportOutsideTheCoroutineCall() {
  UASSERT_MSG(false,
              "current_task::GetCurrentTaskContext() called outside coroutine");
  LOG_ERROR()
      << "current_task::GetCurrentTaskContext() called outside coroutine"
      << logging::LogExtra::Stacktrace();
  throw std::logic_error(
      "current_task::GetCurrentTaskContext() called outside coroutine. "
      "stacktrace:\n" +
      to_string(boost::stacktrace::stacktrace{}));
}

}  // namespace

impl::TaskContext* GetCurrentTaskContext() {
  if (!current_task_context_ptr) {
    ReportOutsideTheCoroutineCall();
  }
  return current_task_context_ptr;
}

impl::TaskContext* GetCurrentTaskContextUnchecked() {
  return current_task_context_ptr;
}

}  // namespace current_task

namespace impl {
namespace {

std::string GetTaskIdString(const TaskContext* task) {
  return std::to_string(task ? task->GetTaskId() : 0);
}

class CurrentTaskScope final {
 public:
  explicit CurrentTaskScope(TaskContext& context, EhGlobals& eh_store)
      : eh_store_(eh_store) {
    current_task::SetCurrentTaskContext(&context);
    ExchangeEhGlobals(eh_store_);
  }

  ~CurrentTaskScope() {
    ExchangeEhGlobals(eh_store_);
    current_task::SetCurrentTaskContext(nullptr);
  }

 private:
  EhGlobals& eh_store_;
};

template <typename Func>
void CallOnce(Func& func) {
  if (func) {
    auto func_to_destroy = std::move(func);
    func_to_destroy();
  }
}

}  // namespace

TaskContext::LocalStorageGuard::LocalStorageGuard(TaskContext& context)
    : context_(context) {
  context_.local_storage_ = &local_storage_;
}

TaskContext::LocalStorageGuard::~LocalStorageGuard() {
  context_.local_storage_ = nullptr;
}

TaskContext::TaskContext(TaskProcessor& task_processor,
                         Task::Importance importance, Payload payload)
    : magic_(kMagic),
      task_processor_(task_processor),
      task_counter_token_(task_processor_.GetTaskCounter()),
      is_critical_(importance == Task::Importance::kCritical),
      payload_(std::move(payload)),
      state_(Task::State::kNew),
      is_detached_(false),
      is_cancellable_(true),
      cancellation_reason_(TaskCancellationReason::kNone),
      finish_waiters_(),
      trace_csw_left_(task_processor_.GetTaskTraceMaxCswForNewTask()),
      wait_manager_(nullptr),
      sleep_state_(SleepStateFlags::kSleeping),
      wakeup_source_(WakeupSource::kNone),
      task_pipe_(nullptr),
      yield_reason_(YieldReason::kNone),
      local_storage_(nullptr) {
  UASSERT(payload_);
  LOG_TRACE() << "task with task_id="
              << GetTaskIdString(current_task::GetCurrentTaskContextUnchecked())
              << " created task with task_id=" << GetTaskIdString(this)
              << logging::LogExtra::Stacktrace();
}

TaskContext::~TaskContext() noexcept {
  LOG_TRACE() << "Task with task_id=" << GetTaskIdString(this) << " stopped"
              << logging::LogExtra::Stacktrace();
}

bool TaskContext::IsCritical() const {
  // running tasks must not be susceptible to overload
  // e.g. we might need to run coroutine to cancel it
  return WasStartedAsCritical() || coro_;
}

void TaskContext::SetDetached() {
  [[maybe_unused]] bool was_detached = is_detached_.exchange(true);
  UASSERT(!was_detached);
}

void TaskContext::Wait() const { WaitUntil({}); }

namespace impl {

class LockedWaitStrategy final : public WaitStrategy {
 public:
  LockedWaitStrategy(Deadline deadline, WaitListLight& waiters,
                     TaskContext& current, const TaskContext& target)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        target_(target) {}

  void AfterAsleep() override {
    waiters_.Append(lock_, &current_);
    if (target_.IsFinished()) waiters_.WakeupOne(lock_);
  }

  void BeforeAwake() override {}

  WaitListBase* GetWaitList() override { return &waiters_; }

 private:
  WaitListLight& waiters_;
  WaitListLight::Lock lock_;
  TaskContext& current_;
  const TaskContext& target_;
};

};  // namespace impl

void TaskContext::WaitUntil(Deadline deadline) const {
  // try to avoid ctx switch if possible
  if (IsFinished()) return;

  UASSERT(current_task::GetCurrentTaskContextUnchecked() != nullptr);

  auto current = current_task::GetCurrentTaskContext();
  if (current->ShouldCancel()) {
    throw WaitInterruptedException(current->cancellation_reason_);
  }

  impl::LockedWaitStrategy wait_manager(deadline, *finish_waiters_, *current,
                                        *this);
  current->Sleep(&wait_manager);

  if (!IsFinished() && current->ShouldCancel()) {
    throw WaitInterruptedException(current->cancellation_reason_);
  }
}

void TaskContext::DoStep() {
  if (IsFinished()) return;

  utils::Flags<SleepStateFlags> clear_flags{SleepStateFlags::kSleeping};
  if (!coro_) {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    coro_ = task_processor_.GetCoroutine();
    clear_flags |= SleepStateFlags::kWakeupByBootstrap;
  }
  // Do non-atomic fetch_and() - we don't care about lost spurious
  // wakeup events
  utils::Flags<SleepStateFlags> new_sleep_state(sleep_state_);
  new_sleep_state.Clear(clear_flags);
  sleep_state_.Store(new_sleep_state, std::memory_order_relaxed);

  // eh_globals is replaced in task scope, we must proxy the exception
  std::exception_ptr uncaught;
  {
    CurrentTaskScope current_task_scope(*this, eh_globals_);
    try {
      SetState(Task::State::kRunning);
      (*coro_)(this);
      if (wait_manager_) wait_manager_->AfterAsleep();
    } catch (...) {
      uncaught = std::current_exception();
    }
  }
  if (uncaught) std::rethrow_exception(uncaught);

  switch (yield_reason_) {
    case YieldReason::kTaskCancelled:
    case YieldReason::kTaskComplete:
      // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
      std::move(coro_).ReturnToPool();
      {
        auto new_state = (yield_reason_ == YieldReason::kTaskComplete)
                             ? Task::State::kCompleted
                             : Task::State::kCancelled;
        SetState(new_state);
        TraceStateTransition(new_state);
      }
      break;

    case YieldReason::kTaskWaiting:
      SetState(Task::State::kSuspended);
      {
        utils::Flags<SleepStateFlags> new_flags = SleepStateFlags::kSleeping;
        if (!IsCancellable()) new_flags |= SleepStateFlags::kNonCancellable;

        // Synchronization point for relaxed SetState()
        auto prev_sleep_state =
            sleep_state_.FetchOr(new_flags, std::memory_order_seq_cst);

        UASSERT(!(prev_sleep_state & SleepStateFlags::kSleeping));
        if (new_flags & SleepStateFlags::kNonCancellable)
          prev_sleep_state.Clear({SleepStateFlags::kWakeupByCancelRequest,
                                  SleepStateFlags::kNonCancellable});
        if (prev_sleep_state) {
          Schedule();
        }
      }
      break;

    case YieldReason::kNone:
      UASSERT(!"invalid yield reason");
      throw std::logic_error("invalid yield reason");
  }
}

void TaskContext::RequestCancel(TaskCancellationReason reason) {
  auto expected = TaskCancellationReason::kNone;
  if (cancellation_reason_.compare_exchange_strong(expected, reason)) {
    LOG_TRACE() << "task with task_id="
                << GetTaskIdString(
                       current_task::GetCurrentTaskContextUnchecked())
                << " cancelled task with task_id=" << GetTaskIdString(this)
                << logging::LogExtra::Stacktrace();
    cancellation_reason_.store(reason, std::memory_order_relaxed);
    Wakeup(WakeupSource::kCancelRequest);
    task_processor_.GetTaskCounter().AccountTaskCancel();
  }
}

bool TaskContext::IsCancellable() const { return is_cancellable_; }

bool TaskContext::SetCancellable(bool value) {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  UASSERT(state_ == Task::State::kRunning);

  return std::exchange(is_cancellable_, value);
}

void TaskContext::Sleep(WaitStrategy* wait_manager) {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  UASSERT(state_ == Task::State::kRunning);

  UASSERT(wait_manager != nullptr);
  /* ConditionVariable might call Sleep() inside of Sleep() due to
   * a lock in AfterAsleep, so store an old value on the stack.
   */
  auto* old_wait_manager = std::exchange(wait_manager_, wait_manager);

  wait_manager_ = wait_manager;
  ev::Timer deadline_timer;
  auto deadline = wait_manager_->GetDeadline();
  if (deadline.IsReachable()) {
    const auto time_left = deadline.TimeLeft();
    if (time_left.count() > 0) {
      boost::intrusive_ptr<TaskContext> ctx(this);
      deadline_timer = ev::Timer(
          task_processor_.EventThreadPool().NextThread(),
          [ctx = std::move(ctx)] { ctx->Wakeup(WakeupSource::kDeadlineTimer); },
          time_left);
    } else {
      Wakeup(WakeupSource::kDeadlineTimer);
    }
  }

  yield_reason_ = YieldReason::kTaskWaiting;
  UASSERT(task_pipe_);
  TraceStateTransition(Task::State::kSuspended);
  ProfilerStopExecution();
  [[maybe_unused]] TaskContext* context = (*task_pipe_)().get();
  ProfilerStartExecution();
  TraceStateTransition(Task::State::kRunning);
  UASSERT(context == this);
  UASSERT(state_ == Task::State::kRunning);

  if (deadline_timer) deadline_timer.Stop();

  if (!(sleep_state_ & SleepStateFlags::kWakeupByWaitList)) {
    auto wait_list = wait_manager_->GetWaitList();
    if (wait_list) {
      wait_list->Remove(this);
    }
  }
  // Clear sleep_state_ now as we may sleep in BeforeAwake() below.
  // Use Load()+Store() instead of Exchange() as seq_cst for RMW operations is
  // very expensive for such hot path.
  auto old_sleep_state = sleep_state_.Load(std::memory_order_acquire);
  sleep_state_.Store(SleepStateFlags::kNone, std::memory_order_relaxed);

  wakeup_source_ = GetPrimaryWakeupSource(old_sleep_state);

  wait_manager_->BeforeAwake();
  wait_manager_ = old_wait_manager;

  // Reset state again for timer firing during wakeup and/or pre-awake doings.
  // All such racy wakeup-ers must be cancelled in BeforeAwake().
  sleep_state_.Store(SleepStateFlags::kNone, std::memory_order_relaxed);
}

bool TaskContext::ShouldSchedule(utils::Flags<SleepStateFlags> prev_flags,
                                 WakeupSource source) {
  /* ShouldSchedule() returns true only for the first Wakeup().  All Wakeup()s
   * are serialized due to seq_cst in FetchOr().
   */

  if (!(prev_flags & SleepStateFlags::kSleeping)) return false;

  if (source == WakeupSource::kCancelRequest) {
    /* Don't wakeup if:
     * 1) kNonCancellable
     * 2) Other WakeupSource is already triggered
     */
    return prev_flags == SleepStateFlags::kSleeping;
  } else if (source == WakeupSource::kBootstrap) {
    return true;
  } else {
    if (prev_flags & SleepStateFlags::kNonCancellable) {
      /* If there was a cancellation request, but cancellation is blocked,
       * ignore it - we're the first to Schedule().
       */
      prev_flags.Clear({SleepStateFlags::kNonCancellable,
                        SleepStateFlags::kWakeupByCancelRequest});
    }

    /* Don't wakeup if:
     * 1) kNonCancellable and zero or more kCancelRequest triggered
     * 2) !kNonCancellable and any other WakeupSource is triggered
     */

    // We're the first to wakeup the baby
    return prev_flags == SleepStateFlags::kSleeping;
  }
}

void TaskContext::Wakeup(WakeupSource source) {
  if (IsFinished()) return;

  if (source == WakeupSource::kCancelRequest &&
      (sleep_state_ & SleepStateFlags::kNonCancellable))
    return;

  /* Set flag regardless of kSleeping - missing kSleeping usually means one of
   * the following: 1) the task is somewhere between Sleep() and setting
   * kSleeping in DoStep(). 2) the task is already awaken, but BeforeAwake() is
   * not yet finished (and not all timers/watchers are stopped).
   */
  auto prev_sleep_state =
      sleep_state_.FetchOr(static_cast<SleepStateFlags>(source));
  if (ShouldSchedule(prev_sleep_state, source)) {
    Schedule();
  }
}

TaskContext::WakeupSource TaskContext::GetWakeupSource() const {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  return wakeup_source_;
}

void TaskContext::CoroFunc(TaskPipe& task_pipe) {
  for (TaskContext* context : task_pipe) {
    UASSERT(context);
    context->yield_reason_ = YieldReason::kNone;
    context->task_pipe_ = &task_pipe;

    context->ProfilerStartExecution();

    // We only let tasks ran with CriticalAsync enter function body, others
    // get terminated ASAP.
    if (context->IsCancelRequested() && !context->WasStartedAsCritical()) {
      context->SetCancellable(false);
      // It is important to destroy payload here as someone may want
      // to synchronize in its dtor (e.g. lambda closure).
      {
        LocalStorageGuard local_storage_guard(*context);
        context->payload_ = {};
      }
      context->yield_reason_ = YieldReason::kTaskCancelled;
    } else {
      try {
        {
          // Destroy contents of LocalStorage in the coroutine
          // as dtors may want to schedule
          LocalStorageGuard local_storage_guard(*context);

          context->TraceStateTransition(Task::State::kRunning);
          CallOnce(context->payload_);
        }
        context->yield_reason_ = YieldReason::kTaskComplete;
      } catch (const CoroUnwinder&) {
        context->yield_reason_ = YieldReason::kTaskCancelled;
      }
    }

    context->ProfilerStopExecution();

    context->task_pipe_ = nullptr;
  }
}

bool TaskContext::HasLocalStorage() const { return local_storage_ != nullptr; }

LocalStorage& TaskContext::GetLocalStorage() { return *local_storage_; }

TaskContext::WakeupSource TaskContext::GetPrimaryWakeupSource(
    utils::Flags<SleepStateFlags> sleep_state) {
  static constexpr std::pair<utils::Flags<SleepStateFlags>, WakeupSource> l[] =
      {{{SleepStateFlags::kWakeupByWaitList}, WakeupSource::kWaitList},
       {{SleepStateFlags::kWakeupByDeadlineTimer},
        WakeupSource::kDeadlineTimer},
       {{SleepStateFlags::kWakeupByBootstrap}, WakeupSource::kBootstrap}};
  for (auto it : l)
    if (sleep_state & it.first) return it.second;

  if ((sleep_state & SleepStateFlags::kWakeupByCancelRequest) &&
      !(sleep_state & SleepStateFlags::kNonCancellable))
    return WakeupSource::kCancelRequest;

  UASSERT_MSG(false, "Cannot find valid wakeup source");
  throw std::logic_error("Cannot find valid wakeup source, stacktrace:\n" +
                         to_string(boost::stacktrace::stacktrace{}) +
                         "\nvalue = " + std::to_string(sleep_state.GetValue()));
}

bool TaskContext::WasStartedAsCritical() const { return is_critical_; }

void TaskContext::SetState(Task::State new_state) {
  auto old_state = Task::State::kNew;

  // CAS optimization
  switch (new_state) {
    case Task::State::kQueued:
      old_state = Task::State::kSuspended;
      break;
    case Task::State::kRunning:
      old_state = Task::State::kQueued;
      break;
    case Task::State::kSuspended:
    case Task::State::kCompleted:
      old_state = Task::State::kRunning;
      break;
    case Task::State::kCancelled:
      old_state = Task::State::kSuspended;
      break;
    case Task::State::kInvalid:
    case Task::State::kNew:
      UASSERT(!"Invalid new task state");
  }

  if (new_state == Task::State::kRunning ||
      new_state == Task::State::kSuspended) {
    if (new_state == Task::State::kRunning) {
      UASSERT(current_task::GetCurrentTaskContext() == this);
    } else {
      UASSERT(current_task::GetCurrentTaskContextUnchecked() == nullptr);
    }
    UASSERT(old_state == state_);
    // For kRunning we don't care other threads see old state_ (kQueued) for
    // some time.
    // For kSuspended synchronization point is DoStep()'s
    // sleep_state_.FetchOr().
    state_.store(new_state, std::memory_order_relaxed);
    return;
  }
  if (new_state == Task::State::kQueued) {
    UASSERT(old_state == state_ || state_ == Task::State::kNew);
    // Synchronization point is TaskProcessor::Schedule()
    state_.store(new_state, std::memory_order_relaxed);
    return;
  }

  // use strong CAS here to avoid losing transitions to finished
  while (!state_.compare_exchange_strong(old_state, new_state)) {
    if (old_state == new_state) {
      // someone else did the job
      return;
    }
    if (old_state == Task::State::kCompleted ||
        old_state == Task::State::kCancelled) {
      // leave as finished, do not wakeup
      return;
    }
  }

  if (IsFinished()) {
    WaitListLight::Lock lock;
    finish_waiters_->WakeupAll(lock);
  }
}

void TaskContext::Schedule() {
  UASSERT(state_ != Task::State::kQueued);
  SetState(Task::State::kQueued);
  TraceStateTransition(Task::State::kQueued);
  task_processor_.Schedule(this);
  // NOTE: may be executed at this point
}

#ifdef USERVER_PROFILER
void TaskContext::ProfilerStartExecution() {
  execute_started_ = std::chrono::steady_clock::now();
}

void TaskContext::ProfilerStopExecution() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now - execute_started_;
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(duration);
  auto threshold_us = task_processor_.GetProfilerThreshold();

  task_processor_.GetTaskCounter().AccountTaskExecution(duration_us);

  if (duration_us >= threshold_us) {
    LOG_ERROR() << "Profiler threshold reached, task was executing "
                   "for too long without context switch ("
                << duration_us.count() << "us >= " << threshold_us.count()
                << "us)" << logging::LogExtra::Stacktrace();
  }
}
#else  // USERVER_PROFILER
void TaskContext::ProfilerStartExecution() {}

void TaskContext::ProfilerStopExecution() {}
#endif

void TaskContext::TraceStateTransition(Task::State state) {
  if (trace_csw_left_ == 0) return;
  --trace_csw_left_;

  auto now = std::chrono::steady_clock::now();
  auto diff = now - last_state_change_timepoint_;
  if (last_state_change_timepoint_ == std::chrono::steady_clock::time_point())
    diff = {};
  auto diff_us =
      std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
  last_state_change_timepoint_ = now;

  auto istate = Task::GetStateName(state);

  LOG_INFO_TO(task_processor_.GetTraceLogger())
      << "Task " << GetTaskId() << " changed state to " << istate
      << ", delay = " << diff_us << "us" << logging::LogExtra::Stacktrace();
}

}  // namespace impl
}  // namespace engine

#include "cxxabi_eh_globals.inc"
