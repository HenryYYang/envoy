#pragma once

#include <string>
#include <map>
#include <list>

#include "envoy/api/api.h"
#include "envoy/config/core/v3/base.pb.h"
#include "envoy/extensions/filters/network/redis_proxy/v3/redis_proxy.pb.h"
#include "envoy/extensions/filters/network/redis_proxy/v3/redis_proxy.pb.validate.h"
#include "envoy/upstream/upstream.h"

// #include "common/common/empty_string.h"
// #include "common/config/datasource.h"

// #include "extensions/filters/network/common/factory_base.h"
// #include "extensions/filters/network/well_known_names.h"


namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

// Fault Injection TODO Priority
// 1. Figure out how to get values off the runtime fraction.


// TODO: Move fault classes
// TODO: Check if there is a runtime hook that updates fault_values. If not, we probably only want to get the config percentages
// every X requests (ie 1000 or more)?

/**
 * Read policy to use for Redis cluster.
 */
enum class FaultType { Delay, Error };

class RedisFaultManager {
  typedef std::multimap<std::string, envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> FaultMapType;

  public:
  RedisFaultManager(Runtime::RandomGenerator& random, Runtime::Loader& runtime, const ::google::protobuf::RepeatedPtrField< ::envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> faults);

  absl::optional<std::pair<FaultType, std::chrono::milliseconds>> get_fault_for_command(std::string command);
  
  private:
  uint64_t calculate_fault_injection_percentage(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);
  std::chrono::milliseconds get_fault_delay_ms(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);
  FaultType get_fault_type(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);

  const std::string ALL_KEY = "ALL_KEYS";
  FaultMapType fault_map_;
  Runtime::RandomGenerator& random_;
  Runtime::Loader& runtime_;
};

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy