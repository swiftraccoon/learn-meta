#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <folly/io/IOBuf.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/ResponseBuilder.h>

namespace learn_meta {

// Configuration constants
constexpr size_t kMaxRequestBodySize = 10 * 1024 * 1024;  // 10MB
constexpr auto kRequestTimeout = std::chrono::seconds(30);
constexpr size_t kMaxHeaderSize = 8192;  // 8KB

/**
 * Health check endpoint handler
 * Responds to GET /health with a JSON status including dependencies
 *
 * @note Uses RAII - automatically cleaned up when request completes
 */
struct HealthHandler : public proxygen::RequestHandler {
 public:
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;
  void requestComplete() noexcept override;
  void onError(proxygen::ProxygenError err) noexcept override;
};

/**
 * Call upload endpoint handler
 * Handles POST /api/call-upload with multipart form data
 *
 * Security:
 * - Validates API key from Authorization header
 * - Enforces request size limits
 * - Validates content type
 *
 * @note Uses RAII - automatically cleaned up when request completes
 */
struct CallUploadHandler : public proxygen::RequestHandler {
 public:
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;
  void requestComplete() noexcept override;
  void onError(proxygen::ProxygenError err) noexcept override;

 private:
  std::unique_ptr<folly::IOBuf> body_;
  size_t bodySize_{0};
  bool isAuthorized_{false};
  std::string apiKey_;
};

/**
 * Main HTTP request handler factory
 * Routes requests to appropriate handlers based on path
 *
 * Features:
 * - Request routing
 * - Metrics collection
 * - Request ID generation
 * - Rate limiting (TODO)
 */
struct HandlerFactory : public proxygen::RequestHandlerFactory {
 public:
  void onServerStart(folly::EventBase* evb) noexcept override;
  void onServerStop() noexcept override;
  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler* handler,
      proxygen::HTTPMessage* msg) noexcept override;

 private:
  std::atomic<uint64_t> requestCount_{0};
  std::atomic<uint64_t> errorCount_{0};
};

/**
 * Validates API key from Authorization header
 * @param headers The HTTP headers to check
 * @return pair of (is_valid, api_key)
 */
std::pair<bool, std::string> validateApiKey(
    const proxygen::HTTPMessage& headers) noexcept;

} // namespace learn_meta