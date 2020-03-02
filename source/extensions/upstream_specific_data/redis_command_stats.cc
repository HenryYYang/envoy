#include "extensions/upstream_specific_data/redis_command_stats.h"

#include "common/stats/timespan_impl.h"

#include "extensions/filters/network/common/redis/supported_commands.h"

namespace Envoy {
namespace Extensions {
namespace UpstreamSpecificData {

// TODO: Pass scope into constructor so we can use it to create timers
RedisCommandStats::RedisCommandStats(Stats::Scope& scope, const std::string& prefix)
    : scope_(scope), symbol_table_(scope_.symbolTable()), stat_name_set_(symbol_table_.makeSet("Redis")),
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

Stats::Counter& RedisCommandStats::counter(const Stats::StatNameVec& stat_names) {
  const Stats::SymbolTable::StoragePtr storage_ptr = symbol_table_.join(stat_names);
  Stats::StatName full_stat_name = Stats::StatName(storage_ptr.get());
  return scope_.counterFromStatName(full_stat_name);
}

Stats::Histogram& RedisCommandStats::histogram(const Stats::StatNameVec& stat_names,
                                               Stats::Histogram::Unit unit) {
  const Stats::SymbolTable::StoragePtr storage_ptr = symbol_table_.join(stat_names);
  Stats::StatName full_stat_name = Stats::StatName(storage_ptr.get());
  return scope_.histogramFromStatName(full_stat_name, unit);
}

Stats::TimespanPtr RedisCommandStats::createCommandTimer(Stats::StatName command, Envoy::TimeSource& time_source) {
  return std::make_unique<Stats::HistogramCompletableTimespanImpl>(
      histogram({prefix_, command, latency_}, Stats::Histogram::Unit::Microseconds),
      time_source);
}

Stats::TimespanPtr RedisCommandStats::createAggregateTimer(Envoy::TimeSource& time_source) {
  return std::make_unique<Stats::HistogramCompletableTimespanImpl>(
      histogram({prefix_, upstream_rq_time_}, Stats::Histogram::Unit::Microseconds),
      time_source);
}

Stats::StatName RedisCommandStats::getCommandFromRequest(const NetworkFilters::Common::Redis::RespValue& request) {
  // Get command from RespValue
  switch (request.type()) {
  case NetworkFilters::Common::Redis::RespType::Array:
    return getCommandFromRequest(request.asArray().front());
  case NetworkFilters::Common::Redis::RespType::CompositeArray:
    return getCommandFromRequest(*request.asCompositeArray().command());
  case NetworkFilters::Common::Redis::RespType::Null:
    return null_metric_;
  case NetworkFilters::Common::Redis::RespType::BulkString:
  case NetworkFilters::Common::Redis::RespType::SimpleString: {
    std::string to_lower_command = absl::AsciiStrToLower(request.asString());
    return stat_name_set_->getBuiltin(to_lower_command, unknown_metric_);
  }
  case NetworkFilters::Common::Redis::RespType::Integer:
  case NetworkFilters::Common::Redis::RespType::Error:
  default:
    return unknown_metric_;
  }
}

void RedisCommandStats::updateStatsTotal(Stats::StatName command) {
  counter({prefix_, command, total_}).inc();
}

void RedisCommandStats::updateStats(Stats::StatName command, const bool success) {
  if (success) {
    counter({prefix_, command, success_}).inc();
  } else {
    counter({prefix_, command, failure_}).inc();
  }
}

} // namespace UpstreamSpecificData
} // namespace Extensions
} // namespace Envoy
