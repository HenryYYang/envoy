#include "extensions/filters/network/common/redis/fault.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

  RedisFaultManager::RedisFaultManager(Runtime::RandomGenerator& random, Runtime::Loader& runtime, const ::google::protobuf::RepeatedPtrField< ::envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault> faults) : random_(random), runtime_(runtime) {
    // Group faults by command
    std::cout << "Inserting faults:" << std::endl;
    for (auto fault : faults) {
      if (fault.commands_size() > 0) {
        for (auto& command: fault.commands()) {
          std::cout << "\t" << "For command: " << absl::AsciiStrToLower(command) << " with type: " << fault.fault_type() << std::endl;
          fault_map_.insert(FaultMapType::value_type(absl::AsciiStrToLower(command), fault));
        }
      } else {
        // Generic "ALL" entry in map
        std::cout << "\t" << "For all: " << ALL_KEY << " with type: " << fault.fault_type() << std::endl;
        fault_map_.insert(FaultMapType::value_type(ALL_KEY, fault));
      }
    }
    std::cout << "fault_map_.size() =" << fault_map_.size() << std::endl;
  }

  absl::optional<std::pair<FaultType, std::chrono::milliseconds>> RedisFaultManager::get_fault_for_command(std::string command) {
    // Fault checking algorithm:
    //
    // For example, if we have an ERROR fault at 5% for all commands, and a DELAY fault at 10% for GET, if
    // we receive a GET, we want 5% of GETs to get DELAY, and 5% to get ERROR. Thus, we need to amortize the
    // percentages.
    //
    // 1. Set amortized_fault to 0
    // 2. Look for command specific faults
    // 3. For each command specific fault
    //  a. check if it applies, using 100 - amortized_fault as modulo for random
    //  b. if (a), return a pair of fault type and delay, if any
    //  c. if (!a), add fault percentage to amortized_fault
    // 4. For each general fault, repeat the process in (3)
    // 5. If we did not hit any faults, return null;

    if (!fault_map_.empty()) {
      int amortized_fault = 0;
      std::pair<FaultMapType::iterator, FaultMapType::iterator> range;

      // TODO: Do we need to create a temporary map? Think about perf.

      // Look for command specific faults first
      std::cout << "\n\n\n" << "get_fault_for_command: " << command << std::endl;

      range = fault_map_.equal_range(command);
      for (FaultMapType::iterator it = range.first; it != range.second; ++it) {
        envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault fault = it->second;
        std::cout << "\t\t" << "for command '" << command << "'" << "with type: " << fault.fault_type() << std::endl;
        const uint64_t fault_injection_percentage = calculate_fault_injection_percentage(fault);
        std::cout << "\t\t\t" << "fault_injection_percentage = " << fault_injection_percentage << ", amortized_fault = " << amortized_fault << std::endl;
        if (random_.random() % (100 - amortized_fault) < fault_injection_percentage) {
          std::cout << "\t\t\t" << "injecting command specific fault" << std::endl;
          return std::make_pair(get_fault_type(fault), get_fault_delay_ms(fault));
        } else {
          amortized_fault += fault_injection_percentage;
        }
      }

      // If that fails, look at faults that apply to all commands
      std::cout << "\t" << "General faults" << " count:" << fault_map_.count(ALL_KEY) << std::endl;
      range = fault_map_.equal_range(ALL_KEY);
      for (FaultMapType::iterator it = range.first; it != range.second; ++it) {
        envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault fault = it->second;
        std::cout << "\t\t" << "---" << "with type: " << fault.fault_type() << std::endl;
        const uint64_t fault_injection_percentage = calculate_fault_injection_percentage(fault);
        std::cout << "\t\t\t" << "fault_injection_percentage = " << fault_injection_percentage << ", amortized_fault = " << amortized_fault << std::endl;
        if (random_.random() % (100 - amortized_fault) < fault_injection_percentage) {
          std::cout << "\t\t\t" << "injecting general fault" << std::endl;
          return std::make_pair(get_fault_type(fault), get_fault_delay_ms(fault));
        } else {
          amortized_fault += fault_injection_percentage;
        }
      }
    }

    // No faults injected!
    return absl::nullopt;
  }

    // TODO: Check if this is ok
  uint64_t RedisFaultManager::calculate_fault_injection_percentage(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault) {
      if (fault.has_fault_enabled()) {
        if (fault.fault_enabled().has_default_value()) {
          envoy::type::v3::FractionalPercent default_value = fault.fault_enabled().default_value();
          if (default_value.denominator() == envoy::type::v3::FractionalPercent::HUNDRED) {
            return runtime_.snapshot().getInteger(fault.fault_enabled().runtime_key(), default_value.numerator());
          } else {
            int denominator = ProtobufPercentHelper::fractionalPercentDenominatorToInt(default_value.denominator());
            int adjusted_numerator = (default_value.numerator() * 100) / denominator;
            return runtime_.snapshot().getInteger(fault.fault_enabled().runtime_key(), adjusted_numerator);
          }
        } else {
          // Default value is zero if not set
          return runtime_.snapshot().getInteger(fault.fault_enabled().runtime_key(), 0);
        }
      } else {
        // Default action- no fault injection
        return 0;
      }
  }

  std::chrono::milliseconds RedisFaultManager::get_fault_delay_ms(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault) {
    return std::chrono::milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(fault, delay, 0));
  }

  FaultType RedisFaultManager::get_fault_type(envoy::extensions::filters::network::redis_proxy::v3::RedisProxy_RedisFault& fault) {
    switch (fault.fault_type()) {
    case envoy::extensions::filters::network::redis_proxy::v3::RedisProxy::RedisFault::DELAY:
      return FaultType::Delay;
    case envoy::extensions::filters::network::redis_proxy::v3::RedisProxy::RedisFault::ERROR:
      return FaultType::Error;
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
      break;
    }
  }

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy