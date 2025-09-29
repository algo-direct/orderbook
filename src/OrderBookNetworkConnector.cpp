

#include "OrderBookNetworkConnector.h"

#include "AsyncIOHeaders.h"
#include "OrderBook.hpp"
#include "OrderBookHTTPClient.h"
#include "OrderBookWsClient.h"
#include "logging.h"

OrderBookNetworkConnector::OrderBookNetworkConnector(std::string_view host,
                                                     std::string_view port)
    : m_ioc{std::make_unique<asio::io_context>()}, m_host{host}, m_port{port} {}

void OrderBookNetworkConnector::reset() {
  LOG_INFO("Resetting..");
  m_orderBook = std::make_unique<OrderBook>();
  //   m_isSnapshotReceived = false;
  auto incrementalUpdateCallback = [&](IncrementalUpdate&& incrementalUpdate) {
    LOG_INFO("incrementalUpdateCallback");
    onIncrementalUpdate(std::move(incrementalUpdate));
  };

  auto disconnectCallback = [&]() { reset(); };
  m_orderBookWsClient = std::make_unique<OrderBookWsClient>(
      incrementalUpdateCallback, disconnectCallback, *m_ioc, m_host, m_port,
      "/ws");
  // DO NOT create or run m_orderBookHTTPClient yet, run once we received first
  // response on websocket
}

void OrderBookNetworkConnector::run() {
  LOG_INFO("Running OrderBookNetworkConnector");
  reset();
  m_orderBookWsClient->run();
  m_ioc->run();
}

void OrderBookNetworkConnector::onIncrementalUpdate(
    IncrementalUpdate&& incrementalUpdate) {
  LOG_INFO("onIncrementalUpdate");
  if (!m_orderBook) {
    return;
  }
  if (!m_orderBookHTTPClient) {
    LOG_INFO("Creating OrderBookHTTPClient ..");
    auto snapshotCallback = [&](OrderBookSnapshot&& orderBookSnapshot) {
      onSnapshot(std::move(orderBookSnapshot));
    };
    auto disconnectCallback = [&]() { reset(); };
    m_orderBookHTTPClient = std::make_unique<OrderBookHTTPClient>(
        snapshotCallback, disconnectCallback, *m_ioc, m_host, m_port,
        "/snapshot");
    m_orderBookHTTPClient->run();
  }
  m_orderBook->applyIncrementalUpdate(std::move(incrementalUpdate));
}

void OrderBookNetworkConnector::onSnapshot(
    OrderBookSnapshot&& orderBookSnapshot) {
  LOG_INFO("Received snapshot");
  if (!m_orderBook) {
    return;
  }
  m_orderBook->applySnapshot(std::move(orderBookSnapshot));
}

OrderBookNetworkConnector::~OrderBookNetworkConnector() = default;