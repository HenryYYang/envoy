#include "common/common/assert.h"

#include "extensions/filters/network/common/redis/fault.h"

#include "test/extensions/filters/network/common/redis/mocks.h"
#include "test/test_common/printers.h"
#include "test/test_common/utility.h"
#include "test/test_common/test_runtime.h"
#include "test/mocks/runtime/mocks.h"

#include "gtest/gtest.h"

using testing::ContainerEq;
using testing::InSequence;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {

using RedisProxy = envoy::extensions::filters::network::redis_proxy::v3::RedisProxy;

class FaultTest : public testing::Test {
    public:

    void createCommandFault(RedisProxy::RedisFault* fault, std::string command_str, int fault_percentage) {
        auto* commands = fault->mutable_commands();
        auto* command = commands->Add();
        command->assign(command_str);
        
        envoy::config::core::v3::RuntimeFractionalPercent* fault_enabled = fault->mutable_fault_enabled();
        fault_enabled->set_runtime_key("runtime_key");
        auto* percentage = fault_enabled->mutable_default_value();
        percentage->set_numerator(fault_percentage);
        percentage->set_denominator(envoy::type::v3::FractionalPercent::HUNDRED);
    }

    testing::NiceMock<Runtime::MockRandomGenerator> random_;
    Runtime::MockLoader runtime_;
};

TEST_F(FaultTest, ConstructFaultMapWithNoFaults) {
    RedisProxy redis_config;
    auto* faults = redis_config.mutable_faults();

    TestScopedRuntime scoped_runtime;
    RedisFaultManager fault_manager = RedisFaultManager(random_, runtime_, *faults);
    ASSERT_EQ(fault_manager.fault_map_.size(), 0);
}

TEST_F(FaultTest, ConstructFaultMapSingleCommandFault) {
    RedisProxy redis_config;
    auto* faults = redis_config.mutable_faults();
    createCommandFault(faults->Add(), "get", 50);
    
    TestScopedRuntime scoped_runtime;
    RedisFaultManager fault_manager = RedisFaultManager(random_, runtime_, *faults);
    ASSERT_EQ(fault_manager.fault_map_.size(), 1);
}

// Single ALL_KEY fault

// Multiple command all_key faults

} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
