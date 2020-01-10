#include <storages/redis/mock_transaction_impl_base_test.hpp>

#include <utils/assert.hpp>

namespace storages {
namespace redis {

// redis commands:

RequestAppend MockTransactionImplBase::Append(std::string /*key*/,
                                              std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestAppend{nullptr};
}

RequestDbsize MockTransactionImplBase::Dbsize(size_t /*shard*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestDbsize{nullptr};
}

RequestDel MockTransactionImplBase::Del(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestDel{nullptr};
}

RequestDel MockTransactionImplBase::Del(std::vector<std::string> /*keys*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestDel{nullptr};
}

RequestExists MockTransactionImplBase::Exists(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestExists{nullptr};
}

RequestExists MockTransactionImplBase::Exists(
    std::vector<std::string> /*keys*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestExists{nullptr};
}

RequestExpire MockTransactionImplBase::Expire(std::string /*key*/,
                                              std::chrono::seconds /*ttl*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestExpire{nullptr};
}

RequestGet MockTransactionImplBase::Get(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGet{nullptr};
}

RequestGetset MockTransactionImplBase::Getset(std::string /*key*/,
                                              std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestGetset{nullptr};
}

RequestHdel MockTransactionImplBase::Hdel(std::string /*key*/,
                                          std::string /*field*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHdel{nullptr};
}

RequestHdel MockTransactionImplBase::Hdel(std::string /*key*/,
                                          std::vector<std::string> /*fields*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHdel{nullptr};
}

RequestHexists MockTransactionImplBase::Hexists(std::string /*key*/,
                                                std::string /*field*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHexists{nullptr};
}

RequestHget MockTransactionImplBase::Hget(std::string /*key*/,
                                          std::string /*field*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHget{nullptr};
}

RequestHgetall MockTransactionImplBase::Hgetall(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHgetall{nullptr};
}

RequestHincrby MockTransactionImplBase::Hincrby(std::string /*key*/,
                                                std::string /*field*/,
                                                int64_t /*increment*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHincrby{nullptr};
}

RequestHincrbyfloat MockTransactionImplBase::Hincrbyfloat(
    std::string /*key*/, std::string /*field*/, double /*increment*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHincrbyfloat{nullptr};
}

RequestHkeys MockTransactionImplBase::Hkeys(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHkeys{nullptr};
}

RequestHlen MockTransactionImplBase::Hlen(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHlen{nullptr};
}

RequestHmget MockTransactionImplBase::Hmget(
    std::string /*key*/, std::vector<std::string> /*fields*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHmget{nullptr};
}

RequestHmset MockTransactionImplBase::Hmset(
    std::string /*key*/,
    std::vector<std::pair<std::string, std::string>> /*field_values*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHmset{nullptr};
}

RequestHset MockTransactionImplBase::Hset(std::string /*key*/,
                                          std::string /*field*/,
                                          std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHset{nullptr};
}

RequestHsetnx MockTransactionImplBase::Hsetnx(std::string /*key*/,
                                              std::string /*field*/,
                                              std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHsetnx{nullptr};
}

RequestHvals MockTransactionImplBase::Hvals(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestHvals{nullptr};
}

RequestIncr MockTransactionImplBase::Incr(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestIncr{nullptr};
}

RequestKeys MockTransactionImplBase::Keys(std::string /*keys_pattern*/,
                                          size_t /*shard*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestKeys{nullptr};
}

RequestLindex MockTransactionImplBase::Lindex(std::string /*key*/,
                                              int64_t /*index*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLindex{nullptr};
}

RequestLlen MockTransactionImplBase::Llen(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLlen{nullptr};
}

RequestLpop MockTransactionImplBase::Lpop(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpop{nullptr};
}

RequestLpush MockTransactionImplBase::Lpush(std::string /*key*/,
                                            std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpush{nullptr};
}

RequestLpush MockTransactionImplBase::Lpush(
    std::string /*key*/, std::vector<std::string> /*values*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLpush{nullptr};
}

RequestLrange MockTransactionImplBase::Lrange(std::string /*key*/,
                                              int64_t /*start*/,
                                              int64_t /*stop*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLrange{nullptr};
}

RequestLrem MockTransactionImplBase::Lrem(std::string /*key*/,
                                          int64_t /*count*/,
                                          std::string /*element*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLrem{nullptr};
}

RequestLtrim MockTransactionImplBase::Ltrim(std::string /*key*/,
                                            int64_t /*start*/,
                                            int64_t /*stop*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestLtrim{nullptr};
}

RequestMget MockTransactionImplBase::Mget(std::vector<std::string> /*keys*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestMget{nullptr};
}

RequestMset MockTransactionImplBase::Mset(
    std::vector<std::pair<std::string, std::string>> /*key_values*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestMset{nullptr};
}

RequestPersist MockTransactionImplBase::Persist(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPersist{nullptr};
}

RequestPexpire MockTransactionImplBase::Pexpire(
    std::string /*key*/, std::chrono::milliseconds /*ttl*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPexpire{nullptr};
}

RequestPing MockTransactionImplBase::Ping(size_t /*shard*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPing{nullptr};
}

RequestPingMessage MockTransactionImplBase::PingMessage(
    size_t /*shard*/, std::string /*message*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestPingMessage{nullptr};
}

RequestRename MockTransactionImplBase::Rename(std::string /*key*/,
                                              std::string /*new_key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRename{nullptr};
}

RequestRpop MockTransactionImplBase::Rpop(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpop{nullptr};
}

RequestRpush MockTransactionImplBase::Rpush(std::string /*key*/,
                                            std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpush{nullptr};
}

RequestRpush MockTransactionImplBase::Rpush(
    std::string /*key*/, std::vector<std::string> /*values*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestRpush{nullptr};
}

RequestSadd MockTransactionImplBase::Sadd(std::string /*key*/,
                                          std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSadd{nullptr};
}

RequestSadd MockTransactionImplBase::Sadd(
    std::string /*key*/, std::vector<std::string> /*members*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSadd{nullptr};
}

RequestScard MockTransactionImplBase::Scard(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestScard{nullptr};
}

RequestSet MockTransactionImplBase::Set(std::string /*key*/,
                                        std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSet{nullptr};
}

RequestSet MockTransactionImplBase::Set(std::string /*key*/,
                                        std::string /*value*/,
                                        std::chrono::milliseconds /*ttl*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSet{nullptr};
}

RequestSetIfExist MockTransactionImplBase::SetIfExist(std::string /*key*/,
                                                      std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfExist{nullptr};
}

RequestSetIfExist MockTransactionImplBase::SetIfExist(
    std::string /*key*/, std::string /*value*/,
    std::chrono::milliseconds /*ttl*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfExist{nullptr};
}

RequestSetIfNotExist MockTransactionImplBase::SetIfNotExist(
    std::string /*key*/, std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfNotExist{nullptr};
}

RequestSetIfNotExist MockTransactionImplBase::SetIfNotExist(
    std::string /*key*/, std::string /*value*/,
    std::chrono::milliseconds /*ttl*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetIfNotExist{nullptr};
}

RequestSetex MockTransactionImplBase::Setex(std::string /*key*/,
                                            std::chrono::seconds /*seconds*/,
                                            std::string /*value*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSetex{nullptr};
}

RequestSismember MockTransactionImplBase::Sismember(std::string /*key*/,
                                                    std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSismember{nullptr};
}

RequestSmembers MockTransactionImplBase::Smembers(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSmembers{nullptr};
}

RequestSrandmember MockTransactionImplBase::Srandmember(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrandmember{nullptr};
}

RequestSrandmembers MockTransactionImplBase::Srandmembers(std::string /*key*/,
                                                          int64_t /*count*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrandmembers{nullptr};
}

RequestSrem MockTransactionImplBase::Srem(std::string /*key*/,
                                          std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrem{nullptr};
}

RequestSrem MockTransactionImplBase::Srem(
    std::string /*key*/, std::vector<std::string> /*members*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestSrem{nullptr};
}

RequestStrlen MockTransactionImplBase::Strlen(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestStrlen{nullptr};
}

RequestTtl MockTransactionImplBase::Ttl(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestTtl{nullptr};
}

RequestType MockTransactionImplBase::Type(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestType{nullptr};
}

RequestZadd MockTransactionImplBase::Zadd(std::string /*key*/, double /*score*/,
                                          std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZadd{nullptr};
}

RequestZadd MockTransactionImplBase::Zadd(std::string /*key*/, double /*score*/,
                                          std::string /*member*/,
                                          const ZaddOptions& /*options*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZadd{nullptr};
}

RequestZaddIncr MockTransactionImplBase::ZaddIncr(std::string /*key*/,
                                                  double /*score*/,
                                                  std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZaddIncr{nullptr};
}

RequestZaddIncrExisting MockTransactionImplBase::ZaddIncrExisting(
    std::string /*key*/, double /*score*/, std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZaddIncrExisting{nullptr};
}

RequestZcard MockTransactionImplBase::Zcard(std::string /*key*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZcard{nullptr};
}

RequestZrangebyscore MockTransactionImplBase::Zrangebyscore(std::string /*key*/,
                                                            double /*min*/,
                                                            double /*max*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscore MockTransactionImplBase::Zrangebyscore(
    std::string /*key*/, std::string /*min*/, std::string /*max*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscore MockTransactionImplBase::Zrangebyscore(
    std::string /*key*/, double /*min*/, double /*max*/,
    const RangeOptions& /*range_options*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscore MockTransactionImplBase::Zrangebyscore(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const RangeOptions& /*range_options*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscore{nullptr};
}

RequestZrangebyscoreWithScores MockTransactionImplBase::ZrangebyscoreWithScores(
    std::string /*key*/, double /*min*/, double /*max*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrangebyscoreWithScores MockTransactionImplBase::ZrangebyscoreWithScores(
    std::string /*key*/, std::string /*min*/, std::string /*max*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrangebyscoreWithScores MockTransactionImplBase::ZrangebyscoreWithScores(
    std::string /*key*/, double /*min*/, double /*max*/,
    const RangeOptions& /*range_options*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrangebyscoreWithScores MockTransactionImplBase::ZrangebyscoreWithScores(
    std::string /*key*/, std::string /*min*/, std::string /*max*/,
    const RangeOptions& /*range_options*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrangebyscoreWithScores{nullptr};
}

RequestZrem MockTransactionImplBase::Zrem(std::string /*key*/,
                                          std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrem{nullptr};
}

RequestZrem MockTransactionImplBase::Zrem(
    std::string /*key*/, std::vector<std::string> /*members*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZrem{nullptr};
}

RequestZscore MockTransactionImplBase::Zscore(std::string /*key*/,
                                              std::string /*member*/) {
  UASSERT_MSG(false, "redis method not mocked");
  return RequestZscore{nullptr};
}

// end of redis commands

}  // namespace redis
}  // namespace storages
