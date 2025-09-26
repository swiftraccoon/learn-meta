#include <glog/logging.h>
#include <gflags/gflags.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include "HttpServer.h"

// Command-line flags with secure defaults
DEFINE_int32(http_port, 8080, "Port to listen on");
DEFINE_int32(threads, 4, "Number of threads to use");
DEFINE_string(ip, "127.0.0.1", "IP address to bind to (default: localhost only)");
DEFINE_bool(enable_http2, true, "Enable HTTP/2 support");
DEFINE_bool(enable_ssl, false, "Enable SSL/TLS (requires cert and key files)");
DEFINE_string(ssl_cert, "", "Path to SSL certificate file");
DEFINE_string(ssl_key, "", "Path to SSL private key file");
DEFINE_int32(idle_timeout_ms, 60000, "Idle timeout in milliseconds");
DEFINE_int32(shutdown_timeout_ms, 10000, "Graceful shutdown timeout in milliseconds");
DEFINE_bool(enable_compression, true, "Enable response compression");

int main(int argc, char* argv[]) {
  // Initialize Google utilities
  folly::init(&argc, &argv, true);
  google::InstallFailureSignalHandler();

  LOG(INFO) << "Starting learn-meta Proxygen gateway";
  LOG(INFO) << "Binding to " << FLAGS_ip << ":" << FLAGS_http_port;
  LOG(INFO) << "HTTP/2 support: " << (FLAGS_enable_http2 ? "enabled" : "disabled");
  LOG(INFO) << "SSL/TLS: " << (FLAGS_enable_ssl ? "enabled" : "disabled");
  LOG(INFO) << "Threads: " << FLAGS_threads;

  // Validate SSL configuration
  if (FLAGS_enable_ssl && (FLAGS_ssl_cert.empty() || FLAGS_ssl_key.empty())) {
    LOG(ERROR) << "SSL enabled but certificate or key file not specified";
    LOG(ERROR) << "Use --ssl_cert and --ssl_key to specify certificate files";
    return 1;
  }

  // Security warning for binding to all interfaces
  if (FLAGS_ip == "0.0.0.0") {
    LOG(WARNING) << "Server binding to all interfaces (0.0.0.0) - "
                 << "this may be a security risk in production";
  }

  try {
    // Configure server IP and protocol
    std::vector<proxygen::HTTPServer::IPConfig> IPs;

    if (FLAGS_enable_ssl) {
      // TODO: Implement SSL configuration
      // For now, log warning and fall back to non-SSL
      LOG(WARNING) << "SSL support not yet implemented, using HTTP";
      IPs.push_back({
        folly::SocketAddress(FLAGS_ip, static_cast<uint16_t>(FLAGS_http_port), true),
        proxygen::HTTPServer::Protocol::HTTP
      });
    } else {
      // Non-SSL configuration
      IPs.push_back({
        folly::SocketAddress(FLAGS_ip, static_cast<uint16_t>(FLAGS_http_port), true),
        proxygen::HTTPServer::Protocol::HTTP
      });
    }

    // Configure server options
    proxygen::HTTPServerOptions options;
    options.threads = static_cast<size_t>(FLAGS_threads);
    options.idleTimeout = std::chrono::milliseconds(FLAGS_idle_timeout_ms);
    options.shutdownOn = {SIGINT, SIGTERM};
    options.enableContentCompression = FLAGS_enable_compression;
    options.h2cEnabled = FLAGS_enable_http2 && !FLAGS_enable_ssl;

    // Set up request handler factory
    options.handlerFactories = proxygen::RequestHandlerChain()
        .addThen<learn_meta::HandlerFactory>()
        .build();

    // Create and start server
    proxygen::HTTPServer server(std::move(options));
    server.bind(IPs);

    // Start server thread
    std::thread serverThread([&server] () {
      LOG(INFO) << "Server starting on thread";
      server.start();
    });

    // Wait for server thread
    serverThread.join();

    LOG(INFO) << "Server shutdown complete";
    return 0;

  } catch (const std::exception& ex) {
    LOG(ERROR) << "Failed to start server: " << ex.what();
    return 1;
  }
}