#pragma once

#include <functional>
#include <memory>
#include <string>

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

class OrderBookSnapshot;
class IncrementalUpdate;
class OrderBookWsClient;
class OrderBookHTTPClient;
class OrderBook;
using OrderBookRef = std::unique_ptr<OrderBook>;

class OrderBookNetworkConnector {
  // OrderBookStateMachine& m_orderBookStateMachine;
  std::unique_ptr<boost::asio::io_context> m_ioc;
  std::string m_host;
  std::string m_port;
  OrderBookRef m_orderBook;
  //   bool m_isSnapshotReceived{false};
  std::unique_ptr<OrderBookWsClient> m_orderBookWsClient;
  std::unique_ptr<OrderBookHTTPClient> m_orderBookHTTPClient;

  void reset();
  void onIncrementalUpdate(IncrementalUpdate&& incrementalUpdate);
  void onSnapshot(OrderBookSnapshot&& orderBookSnapshot);

 public:
  OrderBookNetworkConnector(std::string_view host, std::string_view port);
  ~OrderBookNetworkConnector();
  void run();
};