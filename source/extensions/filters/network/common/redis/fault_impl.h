#pragma once

#include <string>
#include <map>
#include <list>

#include "extensions/filters/network/common/redis/fault.h"
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
 * Fault management- creation, storage and retrieval. Faults are queried for by command,
 * so they are stored in a multimap using the command as key. For faults that apply to
 * all commands, we use a special ALL_KEYS entry in the map.
 */
class RedisFaultManagerImpl : public RedisFaultManager {
  typedef std::multimap<std::string, envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> FaultMapType;

  public:
  RedisFaultManagerImpl(Runtime::RandomGenerator& random, Runtime::Loader& runtime) : random_(random), runtime_(runtime) {}; // For testing
  RedisFaultManagerImpl(Runtime::RandomGenerator& random, 
                    Runtime::Loader& runtime, 
                    const ::google::protobuf::RepeatedPtrField< ::envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> faults);

  absl::optional<std::pair<FaultType, std::chrono::milliseconds>> getFaultForCommand(std::string command) override;
  int numberOfFaults() override;
  
  // Allow the unit test to have access to private members.
  friend class FaultTest;

  private:

  uint64_t calculateFaultInjectionPercentage(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);
  std::chrono::milliseconds getFaultDelayMs(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);
  FaultType getFaultType(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault);

  const std::string ALL_KEY = "ALL_KEY";
  FaultMapType fault_map_;
  protected:
  Runtime::RandomGenerator& random_;
  Runtime::Loader& runtime_;
};

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy