#pragma once

#include <functional>
#include <memory>
#include <string>

using GetSnapshotHandler = std::function<std::string()>;

namespace http::server {
class server;
}

class OrderBookHTTPServer {
  GetSnapshotHandler m_getSnapshotHandler;
  std::unique_ptr<http::server::server> m_httpServer;

 public:
  OrderBookHTTPServer(GetSnapshotHandler getSnapshotHandler,
                      std::string_view host, std::string_view port,
                      std::string_view documentDir);
  ~OrderBookHTTPServer();
  void run();
};
