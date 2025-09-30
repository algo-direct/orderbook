#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

class OrderBookSnapshot;
class IncrementalUpdate;
class WebSocketClient;

using IncrementalUpdateCallback = std::function<void(IncrementalUpdate&&)>;
using DisconnectCallback = std::function<void()>;

class OrderBookWsClient {
  std::string m_host;
  std::string m_port;
  std::string m_uri;
  std::string m_subscriptionRequestJson;
  IncrementalUpdateCallback m_incrementalUpdateCallback;
  DisconnectCallback m_disconnectCallback;
  std::shared_ptr<WebSocketClient> m_webSocketClient;

  void jsonToIncrementalUpdate(std::string_view json,
                               IncrementalUpdate& incrementalUpdate);

 public:
  OrderBookWsClient(IncrementalUpdateCallback incrementalUpdateCallback,
                    DisconnectCallback disconnectCallback,
                    boost::asio::io_context& ioc, std::string_view host,
                    std::string_view port, std::string_view uri);
  void run();
  void stop();
  ~OrderBookWsClient();
};