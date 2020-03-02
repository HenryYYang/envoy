#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace UpstreamSpecificData {

/**
 * Well-known upstream specific data names.
 */
class UpstreamSpecificDataNameValues {
public:
  const std::string RedisCommandStats = "redis_command_stats";
};

using UpstreamSpecificDataNames = ConstSingleton<UpstreamSpecificDataNameValues>;



// TODO: Add a casting method based on name

} // namespace UpstreamSpecificData
} // namespace Extensions
} // namespace Envoy
