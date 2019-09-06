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
  RedisCommandStats(Stats::Scope&, const std::string& prefix, bool enabled);

  static std::shared_ptr<RedisCommandStats>
  createRedisCommandStats(Stats::Scope& stats_scope, const std::string& prefix, bool enabled) {
    return std::make_shared<Common::Redis::RedisCommandStats>(
        stats_scope, prefix,
        enabled);
  }

  Stats::Counter& counter(const Stats::StatNameVec& stat_names);
  Stats::Histogram& histogram(const Stats::StatNameVec& stat_names);
  Stats::CompletableTimespanPtr createCommandTimer(Stats::StatName command,
                                                   Envoy::TimeSource& time_source);
  Stats::CompletableTimespanPtr createAggregateTimer(Envoy::TimeSource& time_source);
  Stats::StatName getCommandFromRequest(const RespValue& request);
  void updateStatsTotal(Stats::StatName command);
  void updateStats(const bool success, Stats::StatName command);
  bool enabled() { return enabled_; }
  Stats::StatName getUnusedStatName() { return unused_metric_; }

private:
  void addCommandToPool(const std::string& command_string);

  Stats::Scope& scope_;
  Stats::StatNamePool stat_name_pool_;
  StringMap<Stats::StatName> stat_name_map_;
  const Stats::StatName prefix_;
  bool enabled_;
  const Stats::StatName upstream_rq_time_;
  const Stats::StatName latency_;
  const Stats::StatName total_;
  const Stats::StatName success_;
  const Stats::StatName error_;
  const Stats::StatName unused_metric_;
  const Stats::StatName null_metric_;
  const Stats::StatName unknown_metric_;
  const ToLowerTable to_lower_table_;
};
using RedisCommandStatsSharedPtr = std::shared_ptr<RedisCommandStats>;

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
