#include "OrderBookNetworkConnector.h"

#include <chrono>
#include <thread>

#include "AsyncIOHeaders.h"
#include "OrderBook.hpp"
#include "OrderBookHTTPClient.h"
#include "OrderBookWsClient.h"
#include "json_utils.h"
#include "logging.h"

OrderBookNetworkConnector::OrderBookNetworkConnector(std::string_view host,
                                                     std::string_view port,
                                                     int reconnectDelay,
                                                     bool useLock)
    : m_ioc{std::make_unique<asio::io_context>()},
      m_host{host},
      m_port{port},
      m_reconnectDelay{reconnectDelay},
      m_spinLock{!useLock},
      m_snapshotReceived{false} {}

void OrderBookNetworkConnector::reset() {
  LOG_INFO("Resetting..");
  SpinLockGaurd spinLockGaurd(m_spinLock);
  m_snapshotReceived = false;
  m_orderBookWsClient.reset();
  m_orderBookHTTPClient.reset();
  m_orderBook = std::make_unique<OrderBook>();
  //   m_isSnapshotReceived = false;
  auto incrementalUpdateCallback = [&](IncrementalUpdate&& incrementalUpdate) {
    LOG_TRACE("incrementalUpdateCallback");
    onIncrementalUpdate(std::move(incrementalUpdate));
  };

  auto disconnectCallback = [&]() {
    LOG_INFO("Sleeping before reconnecting...");
    std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnectDelay));
    LOG_INFO("Reconnecting...");
    reset();
    m_orderBookWsClient->run();
  };
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
  LOG_TRACE("onIncrementalUpdate");
  {
    SpinLockGaurd spinLockGaurd(m_spinLock);
    m_orderBook->applyIncrementalUpdate(std::move(incrementalUpdate));
  }
  if (!m_snapshotReceived && !m_orderBookHTTPClient) {
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
}

void OrderBookNetworkConnector::onSnapshot(
    OrderBookSnapshot&& orderBookSnapshot) {
  LOG_INFO("Received snapshot");
  SpinLockGaurd spinLockGaurd(m_spinLock);
  m_orderBook->applySnapshot(std::move(orderBookSnapshot));
  m_orderBookHTTPClient.reset();
  m_snapshotReceived = true;
}

std::string OrderBookNetworkConnector::getSnapshot() {
  OrderBook orderBook;
  {
    SpinLockGaurd spinLockGaurd(m_spinLock);
    if (!m_orderBook) {
      throw std::runtime_error("orderBook is being reset.");
    }
    // copy order book atomically
    orderBook = *m_orderBook;
  }
  return JsonUtils::orderBookSnapshotToJson(orderBook);
}

OrderBookNetworkConnector::~OrderBookNetworkConnector() = default;