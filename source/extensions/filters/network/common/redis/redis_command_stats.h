#pragma once

#include <memory>
#include <string>

#include "envoy/stats/scope.h"
#include "envoy/stats/timespan.h"

#include "common/stats/symbol_table_impl.h"

#include "extensions/filters/network/common/redis/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

class RedisCommandStats {
public:
  RedisCommandStats(Stats::Scope& scope, const std::string& prefix, bool enabled);

  Stats::Counter& counter(Stats::StatNameVec stat_names);
  Stats::Histogram& histogram(Stats::StatName stat_name);
  Stats::CompletableTimespanPtr createCommandTimer(std::string command,
                                                   Envoy::TimeSource& time_source);
  Stats::CompletableTimespanPtr createAggregateTimer(Envoy::TimeSource& time_source);
  std::string getCommandFromRequest(const RespValue& request);
  void updateStatsTotal(std::string command);
  void updateStats(const bool success, std::string command);
  bool enabled() { return enabled_; }

private:
  void createStats(std::string command);

  Stats::Scope& scope_;
  Stats::StatNameSet stat_name_set_;
  const Stats::StatName prefix_;
  bool enabled_;
  const std::string latency_suffix_ = ".latency";
  const std::string null_metric_ = "null";
  const std::string unknown_metric_ = "unknown";
  const std::string upstream_rq_time_metric_ = "upstream_rq_time";

  const Stats::StatName upstream_rq_time_;
  const Stats::StatName total_;
  const Stats::StatName success_;
  const Stats::StatName error_;
};
using RedisCommandStatsPtr = std::shared_ptr<RedisCommandStats>;

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
