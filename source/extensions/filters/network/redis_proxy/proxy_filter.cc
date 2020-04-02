#include "extensions/filters/network/redis_proxy/proxy_filter.h"

#include <cstdint>
#include <string>

#include "envoy/extensions/filters/network/redis_proxy/v3/redis_proxy.pb.h"
#include "envoy/stats/scope.h"

#include "common/common/assert.h"
#include "common/common/fmt.h"
#include "common/config/datasource.h"
#include "common/config/utility.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace RedisProxy {

ProxyFilterConfig::ProxyFilterConfig(
    const envoy::extensions::filters::network::redis_proxy::v3::RedisProxy& config,
    Stats::Scope& scope, const Network::DrainDecision& drain_decision, Runtime::Loader& runtime,
    Api::Api& api, Runtime::RandomGenerator& random, Event::Dispatcher& dispatcher)
    : drain_decision_(drain_decision), runtime_(runtime),
      stat_prefix_(fmt::format("redis.{}.", config.stat_prefix())),
      stats_(generateStats(stat_prefix_, scope)),
      downstream_auth_password_(
          Config::DataSource::read(config.downstream_auth_password(), true, api)),
      fault_manager_(Common::Redis::RedisFaultManager(random, runtime, config.faults())),
      dispatcher_(dispatcher) {}

ProxyStats ProxyFilterConfig::generateStats(const std::string& prefix, Stats::Scope& scope) {
  return {
      ALL_REDIS_PROXY_STATS(POOL_COUNTER_PREFIX(scope, prefix), POOL_GAUGE_PREFIX(scope, prefix))};
}

ProxyFilter::ProxyFilter(Common::Redis::DecoderFactory& factory,
                         Common::Redis::EncoderPtr&& encoder, CommandSplitter::Instance& splitter,
                         ProxyFilterConfigSharedPtr config)
    : decoder_(factory.create(*this)), encoder_(std::move(encoder)), 
       splitter_(splitter), config_(config) {
  config_->stats_.downstream_cx_total_.inc();
  config_->stats_.downstream_cx_active_.inc();
  connection_allowed_ = config_->downstream_auth_password_.empty();
}

ProxyFilter::~ProxyFilter() {
  ASSERT(pending_requests_.empty());
  config_->stats_.downstream_cx_active_.dec();
}

void ProxyFilter::initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
  callbacks_->connection().addConnectionCallbacks(*this);
  callbacks_->connection().setConnectionStats({config_->stats_.downstream_cx_rx_bytes_total_,
                                               config_->stats_.downstream_cx_rx_bytes_buffered_,
                                               config_->stats_.downstream_cx_tx_bytes_total_,
                                               config_->stats_.downstream_cx_tx_bytes_buffered_,
                                               nullptr, nullptr});
}

void ProxyFilter::onRespValue(Common::Redis::RespValuePtr&& value) {
  std::cout << "ProxyFilter::onRespValue" << std::endl;

  // NOTE: Don't create pending request until we actually need the request.
  pending_requests_.emplace_back(*this);
  PendingRequest& request = pending_requests_.back();

  // Fault injection
  std::string command = "get"; // TODO: Put RedisCommandStats::getCommandFromRequest(...) into utils or resp something
  absl::optional<std::pair<Common::Redis::FaultType, std::chrono::milliseconds>> fault = config_->fault_manager_.get_fault_for_command(command);
  if (fault.has_value()) {
    handleFault(fault.value(), request, std::move(value), false);
  } else {
    onRespValuePostFault(request, std::move(value));
  }
}

void ProxyFilter::onRespValuePostFault(PendingRequest& request, Common::Redis::RespValuePtr&& value) {
  CommandSplitter::SplitRequestPtr split = splitter_.makeRequest(std::move(value), request);
  if (split) {
    // The splitter can immediately respond and destroy the pending request. Only store the handle
    // if the request is still alive.
    request.request_handle_ = std::move(split);
  }
}

void ProxyFilter::handleFault(std::pair<Common::Redis::FaultType, std::chrono::milliseconds> fault, PendingRequest& request, Common::Redis::RespValuePtr&& value, bool delay_performed) {
  // If delay has not been performed...
  if (fault.second > std::chrono::milliseconds(0) && !delay_performed) {
    // TODO:
    // Fire off timer to calll handleFault again, this time with delay_performed set to true.
    // This isn't working meh.
    // Event::Timer delay_timer = config_->dispatcher_.createTimer([this, fault, std::move(request), value]() -> void { handleFault(fault, std::move(request), std::move(value), true); });
    // Event::Timer delay_timer = config_->dispatcher_.createTimer([request_arg = std::move(request), value_arg = std::move(value)] {
    //   handleFault(fault, std::move(request_arg), std::move(value_arg), true);
    // });
    // delay_timer.enableTimer();
    // TODO: Need to hold on to timer so that it can be cancelled if client connection is closed.

    std::cout << "\t" << "--- FAULT INJECTION - [DELAY STARTED] ---" << std::endl;
  } else {
    if (delay_performed) {
      std::cout << "\t" << "--- FAULT INJECTION - [DELAY COMPLETE] ---" << std::endl;
      // TODO: Clean up delay timer
    }

    switch (fault.first) {
      case Common::Redis::FaultType::Delay:
        onRespValuePostFault(request, std::move(value));
        break;
      case Common::Redis::FaultType::Error:
        onErrorFault(request);
        break;
      default:
        NOT_REACHED_GCOVR_EXCL_LINE;
        break;
    }
  }
}

void ProxyFilter::onErrorFault(PendingRequest& request) {
  std::cout << "\t" << "--- FAULT INJECTION - [ERROR] ---" << std::endl;
  Common::Redis::RespValuePtr response{new Common::Redis::RespValue()};
  response->type(Common::Redis::RespType::Error);
  response->asString() = "Fault Injection: Abort";
  request.onResponse(std::move(response));
  request.request_handle_ = nullptr; // just like ping
}

void ProxyFilter::onEvent(Network::ConnectionEvent event) {
  if (event == Network::ConnectionEvent::RemoteClose ||
      event == Network::ConnectionEvent::LocalClose) {
    while (!pending_requests_.empty()) {
      if (pending_requests_.front().request_handle_ != nullptr) {
        pending_requests_.front().request_handle_->cancel();
      }
      pending_requests_.pop_front();
    }
  }
  // TODO: Clean up delay timer
}

void ProxyFilter::onAuth(PendingRequest& request, const std::string& password) {
  Common::Redis::RespValuePtr response{new Common::Redis::RespValue()};
  if (config_->downstream_auth_password_.empty()) {
    response->type(Common::Redis::RespType::Error);
    response->asString() = "ERR Client sent AUTH, but no password is set";
  } else if (password == config_->downstream_auth_password_) {
    response->type(Common::Redis::RespType::SimpleString);
    response->asString() = "OK";
    connection_allowed_ = true;
  } else {
    response->type(Common::Redis::RespType::Error);
    response->asString() = "ERR invalid password";
    connection_allowed_ = false;
  }
  request.onResponse(std::move(response));
}

void ProxyFilter::onResponse(PendingRequest& request, Common::Redis::RespValuePtr&& value) {
  ASSERT(!pending_requests_.empty());
  request.pending_response_ = std::move(value);
  request.request_handle_ = nullptr;

  // The response we got might not be in order, so flush out what we can. (A new response may
  // unlock several out of order responses).
  while (!pending_requests_.empty() && pending_requests_.front().pending_response_) {
    encoder_->encode(*pending_requests_.front().pending_response_, encoder_buffer_);
    pending_requests_.pop_front();
  }

  if (encoder_buffer_.length() > 0) {
    callbacks_->connection().write(encoder_buffer_, false);
  }

  // Check for drain close only if there are no pending responses.
  if (pending_requests_.empty() && config_->drain_decision_.drainClose() &&
      config_->runtime_.snapshot().featureEnabled(config_->redis_drain_close_runtime_key_, 100)) {
    config_->stats_.downstream_cx_drain_close_.inc();
    callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
  }
}

Network::FilterStatus ProxyFilter::onData(Buffer::Instance& data, bool) {
  try {
    decoder_->decode(data);
    return Network::FilterStatus::Continue;
  } catch (Common::Redis::ProtocolError&) {
    config_->stats_.downstream_cx_protocol_error_.inc();
    Common::Redis::RespValue error;
    error.type(Common::Redis::RespType::Error);
    error.asString() = "downstream protocol error";
    encoder_->encode(error, encoder_buffer_);
    callbacks_->connection().write(encoder_buffer_, false);
    callbacks_->connection().close(Network::ConnectionCloseType::NoFlush);
    return Network::FilterStatus::StopIteration;
  }
}

ProxyFilter::PendingRequest::PendingRequest(ProxyFilter& parent) : parent_(parent) {
  parent.config_->stats_.downstream_rq_total_.inc();
  parent.config_->stats_.downstream_rq_active_.inc();
}

ProxyFilter::PendingRequest::~PendingRequest() {
  parent_.config_->stats_.downstream_rq_active_.dec();
}

} // namespace RedisProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
