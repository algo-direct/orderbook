
// #include "OrderBookStateMachine.h"

// #include "OrderBook.hpp"

// OrderBookStateMachine::OrderBookStateMachine(OnResetCallback onResetCallback)
//     : m_onResetCallback{onResetCallback} {
//   reset();
// }
// void OrderBookStateMachine::reset() {
//   // m_orderBookState = OrderBookState::INITIAL;
//   m_orderBook = std::make_unique<OrderBook>();
//   m_isSnapshotReceived = false;
//   m_onResetCallback();
// }

// void OrderBookStateMachine::onError() { reset(); }

// bool OrderBookStateMachine::onIncrementalUpdate(
//     IncrementalUpdate&& incrementalUpdate) {
//   m_orderBook->applyIncrementalUpdate(std::move(incrementalUpdate));
//   return m_isSnapshotReceived;
// }
// void OrderBookStateMachine::onSnapshot(OrderBookSnapshot&& orderBookSnapshot)
// {
//   m_orderBook->applySnapshot(std::move(orderBookSnapshot));
//   m_isSnapshotReceived = true;
// }
