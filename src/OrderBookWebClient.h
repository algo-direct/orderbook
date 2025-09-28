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
class HttpGetter;

using OrderBookSnapshotCallback = std::function<void(OrderBookSnapshot&&)>;

class OrderBookWebClient {
  OrderBookSnapshotCallback m_orderBookSnapshotCallback;
  std::string m_host;
  std::string m_port;
  std::string m_uri;
  int m_httpVersion;
  std::unique_ptr<HttpGetter> m_httpGetter;

  void jsonToOrderBookSnapshot(std::string_view json,
                               OrderBookSnapshot& orderBookSnapshot);
  void jsonToIncrementalUpdate(std::string_view json,
                               IncrementalUpdate& incrementalUpdate);

 public:
  OrderBookWebClient(boost::asio::io_context& ioc, std::string_view host,
                     std::string_view port, std::string_view uri,
                     int httpVersion);
  ~OrderBookWebClient();
  void getSnapshot(OrderBookSnapshot& orderBookSnapshot);
  void getIncrementalUpdate(IncrementalUpdate& incrementalUpdate);
};