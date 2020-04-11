#pragma once

#include <string>
#include <map>
#include <list>

#include "envoy/api/api.h"
#include "envoy/config/core/v3/base.pb.h"
#include "envoy/extensions/filters/network/redis_proxy/v3/redis_proxy.pb.h"
#include "envoy/extensions/filters/network/redis_proxy/v3/redis_proxy.pb.validate.h"
#include "envoy/upstream/upstream.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

/**
 * Read policy to use for Redis cluster.
 */
enum class FaultType { Delay, Error };

/**
 * Fault management- creation, storage and retrieval. Faults are queried for by command,
 * so they are stored in a multimap using the command as key. For faults that apply to
 * all commands, we use a special ALL_KEYS entry in the map.
 */
class RedisFaultManager {
  typedef std::multimap<std::string, envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> FaultMapType;

  public:
  RedisFaultManager(Runtime::RandomGenerator& random, 
                    Runtime::Loader& runtime) : random_(random), runtime_(runtime) {}; // For testing
  RedisFaultManager(Runtime::RandomGenerator& random, 
                    Runtime::Loader& runtime, 
                    const ::google::protobuf::RepeatedPtrField< ::envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> faults);

  absl::optional<std::pair<FaultType, std::chrono::milliseconds>> get_fault_for_command(std::string command);
  
  // Allow the unit test to have access to private members.
  friend class FaultTest;

  private:

  uint64_t calculate_fault_injection_percentage(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);
  std::chrono::milliseconds get_fault_delay_ms(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);
  FaultType get_fault_type(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);

public: // TODO: Remove once we get friend working
  const std::string ALL_KEY = "ALL_KEY";
  FaultMapType fault_map_;
  Runtime::RandomGenerator& random_;
  Runtime::Loader& runtime_;
};

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy