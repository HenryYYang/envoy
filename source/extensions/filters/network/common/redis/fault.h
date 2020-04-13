#pragma once

#include <string>
#include <map>
#include <list>

#include "envoy/api/api.h"
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

class RedisFaultManager {
  public:
  virtual ~RedisFaultManager() = default;

  /**
   * Get fault type and delay given a Redis command.
   * @param command supplies the Redis command string.
   */
  virtual absl::optional<std::pair<FaultType, std::chrono::milliseconds>> getFaultForCommand(std::string command);
  
  /**
   * Get number of faults fault manager has loaded from config.
   */
  virtual int numberOfFaults();
};

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy