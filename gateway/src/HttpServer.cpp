#include "HttpServer.h"

#include <random>
#include <sstream>

#include <folly/Format.h>
#include <folly/Random.h>
#include <folly/json.h>
#include <folly/lang/Exception.h>
#include <glog/logging.h>

namespace learn_meta {

namespace {

// Generate a unique request ID for tracing
std::string generateRequestId() {
  return folly::sformat("{:016x}", folly::Random::rand64());
}

// Check if API key is valid (placeholder - should check against database)
bool isValidApiKey(const std::string& apiKey) {
  // TODO: Implement actual API key validation against database
  // For now, accept keys that start with "sk_" and are at least 32 chars
  return apiKey.size() >= 32 && apiKey.substr(0, 3) == "sk_";
}

} // namespace

// HealthHandler implementation
void HealthHandler::onRequest(
    std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
  auto requestId = generateRequestId();
  headers->getHeaders().add("X-Request-ID", requestId);
  LOG(INFO) << folly::sformat("[{}] Health check requested", requestId);
}

void HealthHandler::onBody(std::unique_ptr<folly::IOBuf> /*body*/) noexcept {
  // Health check doesn't expect body
}

void HealthHandler::onEOM() noexcept {
  try {
    // TODO: Add actual dependency checks (database, cache, etc.)
    folly::dynamic response = folly::dynamic::object
      ("status", "healthy")
      ("service", "learn-meta-gateway")
      ("timestamp", std::time(nullptr))
      ("version", "1.0.0")
      ("dependencies", folly::dynamic::object
        ("database", "healthy")  // Placeholder
        ("cache", "healthy"));    // Placeholder

    proxygen::ResponseBuilder(downstream_)
        .status(200, "OK")
        .header("Content-Type", "application/json")
        .header("Cache-Control", "no-cache")
        .body(folly::toJson(response))
        .sendWithEOM();
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Error in health check: " << ex.what();
    proxygen::ResponseBuilder(downstream_)
        .status(500, "Internal Server Error")
        .body(R"({"error":"Internal server error"})")
        .sendWithEOM();
  }
}

void HealthHandler::onUpgrade(proxygen::UpgradeProtocol /*proto*/) noexcept {
  // Not implemented
}

void HealthHandler::requestComplete() noexcept {
  // Handler is automatically deleted by Proxygen's unique_ptr
}

void HealthHandler::onError(proxygen::ProxygenError err) noexcept {
  LOG(ERROR) << "Health handler error: " << proxygen::getErrorString(err);
}

// CallUploadHandler implementation
void CallUploadHandler::onRequest(
    std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
  auto requestId = generateRequestId();
  headers->getHeaders().add("X-Request-ID", requestId);
  LOG(INFO) << folly::sformat("[{}] Call upload requested: {}",
                               requestId, headers->getPath());

  // Validate method
  if (headers->getMethod() != proxygen::HTTPMethod::POST) {
    proxygen::ResponseBuilder(downstream_)
        .status(405, "Method Not Allowed")
        .header("Content-Type", "application/json")
        .header("X-Request-ID", requestId)
        .body(R"({"error":"Only POST method is allowed"})")
        .sendWithEOM();
    return;
  }

  // Validate authorization
  auto [isValid, apiKey] = validateApiKey(*headers);
  if (!isValid) {
    LOG(WARNING) << folly::sformat("[{}] Unauthorized request", requestId);
    proxygen::ResponseBuilder(downstream_)
        .status(401, "Unauthorized")
        .header("Content-Type", "application/json")
        .header("X-Request-ID", requestId)
        .header("WWW-Authenticate", "Bearer")
        .body(R"({"error":"Invalid or missing API key"})")
        .sendWithEOM();
    return;
  }

  isAuthorized_ = true;
  apiKey_ = std::move(apiKey);

  // Check Content-Length
  auto contentLength = headers->getHeaders().getSingleOrEmpty("Content-Length");
  if (!contentLength.empty()) {
    try {
      size_t length = std::stoull(contentLength);
      if (length > kMaxRequestBodySize) {
        LOG(WARNING) << folly::sformat("[{}] Request too large: {} bytes",
                                       requestId, length);
        proxygen::ResponseBuilder(downstream_)
            .status(413, "Payload Too Large")
            .header("Content-Type", "application/json")
            .header("X-Request-ID", requestId)
            .body(folly::sformat(
                R"({{"error":"Request body too large. Max size: {} bytes"}})",
                kMaxRequestBodySize))
            .sendWithEOM();
        return;
      }
    } catch (const std::exception& ex) {
      LOG(ERROR) << folly::sformat("[{}] Invalid Content-Length: {}",
                                   requestId, ex.what());
    }
  }
}

void CallUploadHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
  if (!isAuthorized_) {
    return; // Already sent error response
  }

  auto bodyLength = body->computeChainDataLength();
  bodySize_ += bodyLength;

  // Check accumulated size
  if (bodySize_ > kMaxRequestBodySize) {
    LOG(WARNING) << folly::sformat("Request body exceeded max size: {} bytes",
                                   bodySize_);
    proxygen::ResponseBuilder(downstream_)
        .status(413, "Payload Too Large")
        .header("Content-Type", "application/json")
        .body(folly::sformat(
            R"({{"error":"Request body too large. Max size: {} bytes"}})",
            kMaxRequestBodySize))
        .sendWithEOM();
    isAuthorized_ = false; // Stop processing
    return;
  }

  if (!body_) {
    body_ = std::move(body);
  } else {
    body_->prependChain(std::move(body));
  }
}

void CallUploadHandler::onEOM() noexcept {
  if (!isAuthorized_) {
    return; // Already sent error response
  }

  try {
    // TODO: Parse multipart form data
    // TODO: Validate required fields
    // TODO: Store in database

    auto requestId = downstream_->getTransaction()->getID();

    folly::dynamic response = folly::dynamic::object
      ("status", "received")
      ("message", "Call upload processed successfully")
      ("requestId", folly::sformat("{}", requestId))
      ("timestamp", std::time(nullptr))
      ("bytesReceived", bodySize_);

    proxygen::ResponseBuilder(downstream_)
        .status(200, "OK")
        .header("Content-Type", "application/json")
        .header("X-Request-ID", folly::sformat("{}", requestId))
        .body(folly::toJson(response))
        .sendWithEOM();

    LOG(INFO) << folly::sformat("[{}] Call upload successful: {} bytes",
                                requestId, bodySize_);
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Error processing call upload: " << ex.what();
    proxygen::ResponseBuilder(downstream_)
        .status(500, "Internal Server Error")
        .header("Content-Type", "application/json")
        .body(R"({"error":"Failed to process upload"})")
        .sendWithEOM();
  }
}

void CallUploadHandler::onUpgrade(
    proxygen::UpgradeProtocol /*proto*/) noexcept {
  // Not implemented
}

void CallUploadHandler::requestComplete() noexcept {
  // Handler is automatically deleted by Proxygen's unique_ptr
}

void CallUploadHandler::onError(proxygen::ProxygenError err) noexcept {
  LOG(ERROR) << "Call upload handler error: " << proxygen::getErrorString(err);
}

// HandlerFactory implementation
void HandlerFactory::onServerStart(folly::EventBase* /*evb*/) noexcept {
  LOG(INFO) << "HTTP server started";
}

void HandlerFactory::onServerStop() noexcept {
  LOG(INFO) << folly::sformat(
      "HTTP server stopped. Total requests: {}, errors: {}",
      requestCount_.load(), errorCount_.load());
}

proxygen::RequestHandler* HandlerFactory::onRequest(
    proxygen::RequestHandler* /*handler*/,
    proxygen::HTTPMessage* msg) noexcept {

  requestCount_++;

  const auto& path = msg->getPath();
  const auto& method = msg->getMethodString();
  auto requestId = generateRequestId();

  LOG(INFO) << folly::sformat("[{}] {} {} from {}",
                              requestId, method, path,
                              msg->getClientAddress().describe());

  // Route to appropriate handler
  if (path == "/health") {
    return new HealthHandler();
  } else if (path == "/api/call-upload" || path == "/call-upload") {
    return new CallUploadHandler();
  } else {
    errorCount_++;

    // Return a 404 handler
    struct NotFoundHandler : public proxygen::RequestHandler {
      void onRequest(std::unique_ptr<proxygen::HTTPMessage>) noexcept override {}
      void onBody(std::unique_ptr<folly::IOBuf>) noexcept override {}
      void onEOM() noexcept override {
        proxygen::ResponseBuilder(downstream_)
            .status(404, "Not Found")
            .header("Content-Type", "application/json")
            .body(R"({"error":"Endpoint not found"})")
            .sendWithEOM();
      }
      void onUpgrade(proxygen::UpgradeProtocol) noexcept override {}
      void requestComplete() noexcept override {}
      void onError(proxygen::ProxygenError) noexcept override {}
    };

    return new NotFoundHandler();
  }
}

// Helper function implementation
std::pair<bool, std::string> validateApiKey(
    const proxygen::HTTPMessage& headers) noexcept {

  auto authHeader = headers.getHeaders().getSingleOrEmpty("Authorization");
  if (authHeader.empty()) {
    return {false, ""};
  }

  // Check for Bearer token
  const std::string bearerPrefix = "Bearer ";
  if (authHeader.size() <= bearerPrefix.size() ||
      authHeader.substr(0, bearerPrefix.size()) != bearerPrefix) {
    return {false, ""};
  }

  auto apiKey = authHeader.substr(bearerPrefix.size());

  // Validate the API key
  if (!isValidApiKey(apiKey)) {
    return {false, ""};
  }

  return {true, apiKey};
}

} // namespace learn_meta