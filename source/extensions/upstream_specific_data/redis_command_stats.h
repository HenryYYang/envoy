#pragma once

#include <memory>
#include <string>

#include "envoy/stats/scope.h"
#include "envoy/stats/timespan.h"
#include "envoy/upstream/upstream.h"

#include "common/stats/symbol_table_impl.h"

#include "extensions/filters/network/common/redis/codec.h"

namespace Envoy {
namespace Extensions {
namespace UpstreamSpecificData {

class RedisCommandStats : public Upstream::ClusterSpecificDatum {
public:
  RedisCommandStats(Stats::Scope& scope, const std::string& prefix);

  static std::shared_ptr<RedisCommandStats>
  createRedisCommandStats(Stats::Scope& scope) {
    return std::make_shared<RedisCommandStats>(scope, "upstream_commands");
  }

  Stats::Counter& counter(const Stats::StatNameVec& stat_names);
  Stats::Histogram& histogram(const Stats::StatNameVec& stat_names,Stats::Histogram::Unit unit);
  Stats::TimespanPtr createCommandTimer(Stats::StatName command, Envoy::TimeSource& time_source);
  Stats::TimespanPtr createAggregateTimer(Envoy::TimeSource& time_source);
  Stats::StatName getCommandFromRequest(const NetworkFilters::Common::Redis::RespValue& request);
  void updateStatsTotal(Stats::StatName command);
  void updateStats(Stats::StatName command, const bool success);
  Stats::StatName getUnusedStatName() { return unused_metric_; }

private:
  Stats::Scope& scope_;
  Stats::SymbolTable& symbol_table_;
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
};
using RedisCommandStatsSharedPtr = std::shared_ptr<RedisCommandStats>;

} // namespace UpstreamSpecificData
} // namespace Extensions
} // namespace Envoy
