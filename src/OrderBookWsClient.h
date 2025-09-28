#pragma once

#include <memory>
#include <string>

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

class OrderBookSnapshot;
class IncrementalUpdate;
class WebSocketClient;

class OrderBookWsClient {
  std::string m_host;
  std::string m_port;
  std::string m_uri;
  int m_httpVersion;
  std::unique_ptr<WebSocketClient> m_webSocketClient;

  void jsonToIncrementalUpdate(std::string_view json,
                               IncrementalUpdate& incrementalUpdate);

 public:
  OrderBookWsClient(boost::asio::io_context& ioc, std::string_view host,
                    std::string_view port, std::string_view uri,
                    int httpVersion);
  void sendSubscriptionRequest();
};