#include "extensions/filters/network/common/redis/redis_command_stats.h"

#include "common/stats/timespan_impl.h"

#include "extensions/filters/network/common/redis/supported_commands.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

RedisCommandStats::RedisCommandStats(Stats::Scope& scope, const std::string& prefix)
    : scope_(scope), stat_name_set_(scope.symbolTable().makeSet("Redis")),
      prefix_(stat_name_set_->add(prefix)),
      upstream_rq_time_(stat_name_set_->add("upstream_rq_time")),
      latency_(stat_name_set_->add("latency")), total_(stat_name_set_->add("total")),
      success_(stat_name_set_->add("success")), failure_(stat_name_set_->add("failure")),
      unused_metric_(stat_name_set_->add("unused")), null_metric_(stat_name_set_->add("null")),
      unknown_metric_(stat_name_set_->add("unknown")) {
  // Note: Even if this is disabled, we track the upstream_rq_time.
  // Create StatName for each Redis command. Note that we don't include Auth or Ping.
  stat_name_set_->rememberBuiltins(
      Extensions::NetworkFilters::Common::Redis::SupportedCommands::simpleCommands());
  stat_name_set_->rememberBuiltins(
      Extensions::NetworkFilters::Common::Redis::SupportedCommands::evalCommands());
  stat_name_set_->rememberBuiltins(Extensions::NetworkFilters::Common::Redis::SupportedCommands::
                                       hashMultipleSumResultCommands());
  stat_name_set_->rememberBuiltin(
      Extensions::NetworkFilters::Common::Redis::SupportedCommands::mget());
  stat_name_set_->rememberBuiltin(
      Extensions::NetworkFilters::Common::Redis::SupportedCommands::mset());
}

Stats::Counter& RedisCommandStats::counter(Stats::Scope& scope,
                                           const Stats::StatNameVec& stat_names) const {
  const Stats::SymbolTable::StoragePtr storage_ptr = scope_.symbolTable().join(stat_names);
  Stats::StatName full_stat_name = Stats::StatName(storage_ptr.get());
  return scope.counterFromStatName(full_stat_name);
}

Stats::Histogram& RedisCommandStats::histogram(Stats::Scope& scope,
                                               const Stats::StatNameVec& stat_names,
                                               Stats::Histogram::Unit unit) const {
  const Stats::SymbolTable::StoragePtr storage_ptr = scope_.symbolTable().join(stat_names);
  Stats::StatName full_stat_name = Stats::StatName(storage_ptr.get());
  return scope.histogramFromStatName(full_stat_name, unit);
}

Stats::TimespanPtr RedisCommandStats::createCommandTimer(Stats::Scope& scope,
                                                         Stats::StatName command,
                                                         Envoy::TimeSource& time_source) const {
  return std::make_unique<Stats::HistogramCompletableTimespanImpl>(
      histogram(scope, {prefix_, command, latency_}, Stats::Histogram::Unit::Microseconds),
      time_source);
}

Stats::TimespanPtr RedisCommandStats::createAggregateTimer(Stats::Scope& scope,
                                                           Envoy::TimeSource& time_source) const {
  return std::make_unique<Stats::HistogramCompletableTimespanImpl>(
      histogram(scope, {prefix_, upstream_rq_time_}, Stats::Histogram::Unit::Microseconds),
      time_source);
}

Stats::StatName RedisCommandStats::getCommandFromRequest(const RespValue& request) const {
  // Get command from RespValue
  switch (request.type()) {
  case RespType::Array:
    return getCommandFromRequest(request.asArray().front());
  case RespType::Integer:
    return unknown_metric_;
  case RespType::Null:
    return null_metric_;
  default:
    std::string to_lower_command(request.asString());
    to_lower_table_.toLowerCase(to_lower_command);
    return stat_name_set_->getBuiltin(to_lower_command, unknown_metric_);
  }
}

void RedisCommandStats::updateStatsTotal(Stats::Scope& scope, Stats::StatName command) const {
  counter(scope, {prefix_, command, total_}).inc();
}

void RedisCommandStats::updateStats(Stats::Scope& scope, Stats::StatName command,
                                    const bool success) const {
  if (success) {
    counter(scope, {prefix_, command, success_}).inc();
  } else {
    counter(scope, {prefix_, command, failure_}).inc();
  }
}

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
