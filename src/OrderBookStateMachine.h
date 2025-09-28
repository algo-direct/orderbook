#pragma once

#include <functional>
#include <memory>

class OrderBook;

using OrderBookRef = std::unique_ptr<OrderBook>;

// enum class OrderBookState {
//   INITIAL,
//   SUBSCRIPTION_REQUEST_SENT,
// //   SNAPSHOT_REQUEST_SENT,
//   SNAPSHOT_RESPONSE_RECEIVED,
// };

class OrderBookSnapshot;
class IncrementalUpdate;

using OnResetCallback = std::function<void()>;

class OrderBookStateMachine {
  //   OrderBookState m_orderBookState{OrderBookState::INITIAL};
  OnResetCallback m_onResetCallback;
  OrderBookRef m_orderBook;
  bool m_isSnapshotReceived{false};

  void reset();

 public:
  OrderBookStateMachine();

  void onError();

  // return false if snapshot not received yet
  bool onIncrementalUpdate(IncrementalUpdate&& incrementalUpdate);
  void onSnapshot(OrderBookSnapshot&& orderBookSnapshot);
};