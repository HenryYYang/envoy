#pragma once

#include <memory>
#include <string>

#include "envoy/stats/scope.h"
#include "envoy/stats/timespan.h"

#include "common/common/to_lower_table.h"
#include "common/stats/symbol_table_impl.h"

#include "extensions/filters/network/common/redis/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

class RedisCommandStats {
public:
  RedisCommandStats(Stats::Scope& scope, const std::string& prefix);

  // TODO (@FAYiEKcbD0XFqF2QK2E4viAHg8rMm2VbjYKdjTg): Figure out how to make sure command splitter
  // behaves correctly with respect to redis command stats
  static std::shared_ptr<RedisCommandStats> createRedisCommandStats(Stats::Scope& scope) {
    return std::make_shared<Common::Redis::RedisCommandStats>(scope, "upstream_commands");
  }

  Stats::Counter& counter(Stats::Scope& scope, const Stats::StatNameVec& stat_names) const;
  Stats::Histogram& histogram(Stats::Scope& scope, const Stats::StatNameVec& stat_names,
                              Stats::Histogram::Unit unit) const;
  Stats::TimespanPtr createCommandTimer(Stats::Scope& scope, Stats::StatName command,
                                        Envoy::TimeSource& time_source) const;
  Stats::TimespanPtr createAggregateTimer(Stats::Scope& scope,
                                          Envoy::TimeSource& time_source) const;
  Stats::StatName getCommandFromRequest(const RespValue& request) const;
  void updateStatsTotal(Stats::Scope& scope, Stats::StatName command) const;
  void updateStats(Stats::Scope& scope, Stats::StatName command, const bool success) const;
  Stats::StatName getUnusedStatName() const { return unused_metric_; }

private:
  Stats::Scope& scope_;
  Stats::StatNameSetPtr stat_name_set_;
  const Stats::StatName prefix_;
  const Stats::StatName upstream_rq_time_;
  const Stats::StatName latency_;
  const Stats::StatName total_;
  const Stats::StatName success_;
  const Stats::StatName failure_;
  const Stats::StatName unused_metric_;
  const Stats::StatName null_metric_;
  const Stats::StatName unknown_metric_;
  const ToLowerTable to_lower_table_;
};
using RedisCommandStatsSharedPtr = std::shared_ptr<RedisCommandStats>;

/**
 * Factory class that we register with upstream ClusterInfo map of upstream-specific data.
 */
class RedisCommandStatsFactory {
public:
  static const std::string UPSTREAM_MAP_KEY;

  const RedisCommandStatsSharedPtr getOrCreateRedisCommandStats(Stats::Scope& scope) {
    Thread::LockGuard lock(mutex_);
    if (redis_command_stats_ == nullptr) {
      redis_command_stats_ = std::make_shared<RedisCommandStats>(scope, prefix_);
    }
    return redis_command_stats_;
  }

private:
  const std::string prefix_ = "upstream_commands";
  mutable Thread::MutexBasicLockable mutex_;
  RedisCommandStatsSharedPtr redis_command_stats_;
};

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
