#pragma once

#include <engine/future.hpp>

#include <storages/redis/mock_transaction_impl_base_test.hpp>
#include <storages/redis/transaction.hpp>

namespace storages {
namespace redis {

class MockClientBase;

class MockTransaction final : public Transaction {
 public:
  MockTransaction(std::shared_ptr<MockClientBase> client,
                  std::unique_ptr<MockTransactionImplBase> impl,
                  CheckShards check_shards = CheckShards::kSame);

  ~MockTransaction();

  RequestExec Exec(const CommandControl& command_control) override;

  // redis commands:

  RequestAppend Append(std::string key, std::string value) override;

  RequestDbsize Dbsize(size_t shard) override;

  RequestDel Del(std::string key) override;

  RequestDel Del(std::vector<std::string> keys) override;

  RequestExists Exists(std::string key) override;

  RequestExists Exists(std::vector<std::string> keys) override;

  RequestExpire Expire(std::string key, std::chrono::seconds ttl) override;

  RequestGet Get(std::string key) override;

  RequestGetset Getset(std::string key, std::string value) override;

  RequestHdel Hdel(std::string key, std::string field) override;

  RequestHdel Hdel(std::string key, std::vector<std::string> fields) override;

  RequestHexists Hexists(std::string key, std::string field) override;

  RequestHget Hget(std::string key, std::string field) override;

  RequestHgetall Hgetall(std::string key) override;

  RequestHincrby Hincrby(std::string key, std::string field,
                         int64_t increment) override;

  RequestHincrbyfloat Hincrbyfloat(std::string key, std::string field,
                                   double increment) override;

  RequestHkeys Hkeys(std::string key) override;

  RequestHlen Hlen(std::string key) override;

  RequestHmget Hmget(std::string key, std::vector<std::string> fields) override;

  RequestHmset Hmset(
      std::string key,
      std::vector<std::pair<std::string, std::string>> field_values) override;

  RequestHset Hset(std::string key, std::string field,
                   std::string value) override;

  RequestHsetnx Hsetnx(std::string key, std::string field,
                       std::string value) override;

  RequestHvals Hvals(std::string key) override;

  RequestIncr Incr(std::string key) override;

  RequestKeys Keys(std::string keys_pattern, size_t shard) override;

  RequestLindex Lindex(std::string key, int64_t index) override;

  RequestLlen Llen(std::string key) override;

  RequestLpop Lpop(std::string key) override;

  RequestLpush Lpush(std::string key, std::string value) override;

  RequestLpush Lpush(std::string key, std::vector<std::string> values) override;

  RequestLrange Lrange(std::string key, int64_t start, int64_t stop) override;

  RequestLrem Lrem(std::string key, int64_t count,
                   std::string element) override;

  RequestLtrim Ltrim(std::string key, int64_t start, int64_t stop) override;

  RequestMget Mget(std::vector<std::string> keys) override;

  RequestMset Mset(
      std::vector<std::pair<std::string, std::string>> key_values) override;

  RequestPersist Persist(std::string key) override;

  RequestPexpire Pexpire(std::string key,
                         std::chrono::milliseconds ttl) override;

  RequestPing Ping(size_t shard) override;

  RequestPingMessage PingMessage(size_t shard, std::string message) override;

  RequestRename Rename(std::string key, std::string new_key) override;

  RequestRpop Rpop(std::string key) override;

  RequestRpush Rpush(std::string key, std::string value) override;

  RequestRpush Rpush(std::string key, std::vector<std::string> values) override;

  RequestSadd Sadd(std::string key, std::string member) override;

  RequestSadd Sadd(std::string key, std::vector<std::string> members) override;

  RequestScard Scard(std::string key) override;

  RequestSet Set(std::string key, std::string value) override;

  RequestSet Set(std::string key, std::string value,
                 std::chrono::milliseconds ttl) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value) override;

  RequestSetIfExist SetIfExist(std::string key, std::string value,
                               std::chrono::milliseconds ttl) override;

  RequestSetIfNotExist SetIfNotExist(std::string key,
                                     std::string value) override;

  RequestSetIfNotExist SetIfNotExist(std::string key, std::string value,
                                     std::chrono::milliseconds ttl) override;

  RequestSetex Setex(std::string key, std::chrono::seconds seconds,
                     std::string value) override;

  RequestSismember Sismember(std::string key, std::string member) override;

  RequestSmembers Smembers(std::string key) override;

  RequestSrandmember Srandmember(std::string key) override;

  RequestSrandmembers Srandmembers(std::string key, int64_t count) override;

  RequestSrem Srem(std::string key, std::string member) override;

  RequestSrem Srem(std::string key, std::vector<std::string> members) override;

  RequestStrlen Strlen(std::string key) override;

  RequestTtl Ttl(std::string key) override;

  RequestType Type(std::string key) override;

  RequestZadd Zadd(std::string key, double score, std::string member) override;

  RequestZadd Zadd(std::string key, double score, std::string member,
                   const ZaddOptions& options) override;

  RequestZaddIncr ZaddIncr(std::string key, double score,
                           std::string member) override;

  RequestZaddIncrExisting ZaddIncrExisting(std::string key, double score,
                                           std::string member) override;

  RequestZcard Zcard(std::string key) override;

  RequestZrangebyscore Zrangebyscore(std::string key, double min,
                                     double max) override;

  RequestZrangebyscore Zrangebyscore(std::string key, std::string min,
                                     std::string max) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, double min, double max,
      const RangeOptions& range_options) override;

  RequestZrangebyscore Zrangebyscore(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(std::string key,
                                                         double min,
                                                         double max) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, double min, double max,
      const RangeOptions& range_options) override;

  RequestZrangebyscoreWithScores ZrangebyscoreWithScores(
      std::string key, std::string min, std::string max,
      const RangeOptions& range_options) override;

  RequestZrem Zrem(std::string key, std::string member) override;

  RequestZrem Zrem(std::string key, std::vector<std::string> members) override;

  RequestZscore Zscore(std::string key, std::string member) override;

  // end of redis commands

 private:
  class ResultPromise;
  class MockRequestExecDataImpl;

  void UpdateShard(const std::string& key);
  void UpdateShard(const std::vector<std::string>& keys);
  void UpdateShard(
      const std::vector<std::pair<std::string, std::string>>& key_values);
  void UpdateShard(size_t shard);

  template <typename Result, typename ReplyType>
  Request<Result, ReplyType> AddSubrequest(
      Request<Result, ReplyType>&& subrequest);

  RequestExec CreateMockExecRequest();

  std::shared_ptr<MockClientBase> client_;
  const CheckShards check_shards_;

  std::unique_ptr<MockTransactionImplBase> impl_;

  boost::optional<size_t> shard_;
  std::vector<std::unique_ptr<ResultPromise>> result_promises_;
};

}  // namespace redis
}  // namespace storages
