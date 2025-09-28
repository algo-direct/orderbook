#pragma once

class OrderBookSnapshot;
class IncrementalUpdate;

class OrderBookWsClient {
  std::string m_host;
  std::string m_port;

  void jsonToOrderBookSnapshot(std::string_view json,
                               OrderBookSnapshot& orderBookSnapshot);
  void jsonToIncrementalUpdate(std::string_view json,
                               IncrementalUpdate& incrementalUpdate);

 public:
  OrderBookWsClient(std::string_view host, std::string_view port);
  void getSnapshot(OrderBookSnapshot& orderBookSnapshot);
  void getIncrementalUpdate(IncrementalUpdate& incrementalUpdate);
};