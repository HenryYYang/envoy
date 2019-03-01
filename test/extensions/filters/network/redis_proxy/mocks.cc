#include "mocks.h"

#include <cstdint>

#include "common/common/assert.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace RedisProxy {

void PrintTo(const Common::Redis::RespValue& value, std::ostream* os) { *os << value.toString(); }

void PrintTo(const Common::Redis::RespValuePtr& value, std::ostream* os) { *os << value->toString(); }

bool operator==(const Common::Redis::RespValue& lhs, const Common::Redis::RespValue& rhs) {
  if (lhs.type() != rhs.type()) {
    return false;
  }

  switch (lhs.type()) {
  case Common::Redis::RespType::Array: {
    if (lhs.asArray().size() != rhs.asArray().size()) {
      return false;
    }

    bool equal = true;
    for (uint64_t i = 0; i < lhs.asArray().size(); i++) {
      equal &= (lhs.asArray()[i] == rhs.asArray()[i]);
    }

    return equal;
  }
  case Common::Redis::RespType::SimpleString:
  case Common::Redis::RespType::BulkString:
  case Common::Redis::RespType::Error: {
    return lhs.asString() == rhs.asString();
  }
  case Common::Redis::RespType::Null: {
    return true;
  }
  case Common::Redis::RespType::Integer: {
    return lhs.asInteger() == rhs.asInteger();
  }
  }

  NOT_REACHED_GCOVR_EXCL_LINE;
}

MockEncoder::MockEncoder() {
  ON_CALL(*this, encode(_, _))
      .WillByDefault(Invoke([this](const Common::Redis::RespValue& value, Buffer::Instance& out) -> void {
        real_encoder_.encode(value, out);
      }));
}

MockEncoder::~MockEncoder() {}

MockDecoder::MockDecoder() {}
MockDecoder::~MockDecoder() {}

namespace ConnPool {

MockClient::MockClient() {
  ON_CALL(*this, addConnectionCallbacks(_))
      .WillByDefault(Invoke([this](Network::ConnectionCallbacks& callbacks) -> void {
        callbacks_.push_back(&callbacks);
      }));
  ON_CALL(*this, close()).WillByDefault(Invoke([this]() -> void {
    raiseEvent(Network::ConnectionEvent::LocalClose);
  }));
}

MockClient::~MockClient() {}

MockPoolRequest::MockPoolRequest() {}
MockPoolRequest::~MockPoolRequest() {}

MockPoolCallbacks::MockPoolCallbacks() {}
MockPoolCallbacks::~MockPoolCallbacks() {}

MockInstance::MockInstance() {}
MockInstance::~MockInstance() {}

} // namespace ConnPool

namespace CommandSplitter {

MockSplitRequest::MockSplitRequest() {}
MockSplitRequest::~MockSplitRequest() {}

MockSplitCallbacks::MockSplitCallbacks() {}
MockSplitCallbacks::~MockSplitCallbacks() {}

MockInstance::MockInstance() {}
MockInstance::~MockInstance() {}

} // namespace CommandSplitter
} // namespace RedisProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
