
#include "OrderBookNetworkConnector.h"

#include "AsyncIOHeaders.h"
#include "OrderBook.hpp"
#include "OrderBookStateMachine.h"
#include "OrderBookWebClient.h"
#include "OrderBookWsClient.h"

OrderBookNetworkConnector::OrderBookNetworkConnector(std::string_view host,
                                                     std::string_view port)
    : m_ioc{std::make_unique<asio::io_context>()}, m_host{host}, m_port{port} {
  reset();
}

void OrderBookNetworkConnector::reset() {
  m_orderBook = std::make_unique<OrderBook>();
  m_isSnapshotReceived = false;
  auto incrementalUpdateCallback =
      [&](IncrementalUpdate&& incrementalUpdate) {
        onIncrementalUpdate(std::move(incrementalUpdate));
      };
  auto snapshotCallback = [&](OrderBookSnapshot&& orderBookSnapshot) {
    onSnapshot(std::move(orderBookSnapshot));
  };

  auto disconnectCallback = [&]() { reset(); };
  m_orderBookWsClient = std::make_unique<OrderBookWsClient>(
    incrementalUpdateCallback,
    disconnectCallback,
    *m_ioc, m_host, m_port, "/ws", httpVersion
  );
    
}

void OrderBookNetworkConnector::run() {}

void OrderBookNetworkConnector::onIncrementalUpdate(
    IncrementalUpdate&& incrementalUpdate) {}
void OrderBookNetworkConnector::onSnapshot(
    OrderBookSnapshot&& orderBookSnapshot) {}

OrderBookNetworkConnector::~OrderBookNetworkConnector() = default;