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
  RedisCommandStats(Stats::Scope& scope, const std::string& prefix, bool enableCommandCounts,
                    bool latency_in_micros);

  Stats::Counter& counter(std::string name);
  Stats::Histogram& histogram(std::string name);
  Stats::Histogram& histogram(Stats::StatName stat_name);
  Stats::CompletableTimespanPtr createTimer(std::string name, Envoy::TimeSource& time_source);
  Stats::CompletableTimespanPtr createTimer(Stats::StatName stat_name,
                                            Envoy::TimeSource& time_source);
  void incrementCounter(const RespValue& request);

private:
  void createStats(std::string name);
  Stats::SymbolTable::StoragePtr addPrefix(const Stats::StatName name);

  Stats::Scope& scope_;
  Stats::StatNameSet stat_name_set_;
  const Stats::StatName prefix_;
  bool latency_in_micros_;

public:
  const Stats::StatName upstream_rq_time_;
};
using RedisCommandStatsPtr = std::shared_ptr<RedisCommandStats>;

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
