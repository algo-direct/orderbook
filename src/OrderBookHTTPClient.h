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
class HttpGetter;

using OrderBookSnapshotCallback = std::function<void(OrderBookSnapshot&&)>;
using ErrorCallback = std::function<void()>;

class OrderBookHTTPClient {
  OrderBookSnapshotCallback m_orderBookSnapshotCallback;
  ErrorCallback m_errorCallback;
  std::string m_host;
  std::string m_port;
  std::string m_uri;
  int m_httpVersion;
  std::shared_ptr<HttpGetter> m_httpGetter;

  void jsonToOrderBookSnapshot(std::string_view json,
                               OrderBookSnapshot& orderBookSnapshot);

 public:
  OrderBookHTTPClient(OrderBookSnapshotCallback orderBookSnapshotCallback,
                      ErrorCallback errorCallback, boost::asio::io_context& ioc,
                      std::string_view host, std::string_view port,
                      std::string_view uri);
  ~OrderBookHTTPClient();
  void run();
  void stop();
};