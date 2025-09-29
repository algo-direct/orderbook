#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <map>
#include <queue>
#include <vector>

#include "common_header.h"
#include "logging.h"
#include "utils.h"

class OrderBook {
  SequenceType m_sequence{0};
  TimePoint m_lastUpdateTimestamp;
  bool m_snapshotReceived{false};
  BidLevels m_bids;
  AskLevels m_asks;
  std::queue<IncrementalUpdate> m_pendingIncrementalUpdates;

 public:
  void applySnapshot(OrderBookSnapshot&& orderBookSnapshot) {
    m_snapshotReceived = true;
    for (auto& level : orderBookSnapshot.bids) {
      level.sequence = m_sequence;
      m_bids[level.price] = level;
    }
    for (auto& level : orderBookSnapshot.asks) {
      level.sequence = m_sequence;
      m_asks[level.price] = level;
    }

    while (!m_pendingIncrementalUpdates.empty()) {
      applyIncrementalUpdate(std::move(m_pendingIncrementalUpdates.front()));
      m_pendingIncrementalUpdates.pop();
    }

    m_lastUpdateTimestamp = orderBookSnapshot.timestamp;
    m_sequence = orderBookSnapshot.sequence;
  }

  template <typename LevelType>
  void applyLevels(const Levels& inputLevels, LevelType& levels) {
    for (auto& inputLevel : inputLevels) {
      if (inputLevel.sequence <= m_sequence) {
        continue;
      }
      if (sizeCompareEqual(inputLevel.size, 0)) {
        levels.erase(inputLevel.price);
        continue;
      }
      auto insertResult = levels.insert({inputLevel.price, inputLevel});
      if (!insertResult.second) {
        auto& level = insertResult.first->second;
        if (inputLevel.sequence <= level.sequence) {
          continue;
        }
        //  If the price is 0, ignore the messages and update the sequence.
        if (priceCompareEqual(inputLevel.price, 0)) {
          level.sequence = inputLevel.sequence;
          continue;
        }
        level.sequence = inputLevel.sequence;
        level.size = inputLevel.size;
        level.price = inputLevel.price;
      }
    }
  }

  bool applyIncrementalUpdate(IncrementalUpdate&& incrementalUpdate) {
    if (!m_snapshotReceived) {
      // snapshot not received yet
      if (std::size(m_pendingIncrementalUpdates) == 0) {
        // this is the first incremental updates, use the start sequence
        m_sequence = incrementalUpdate.sequenceStart - 1;
      }
      m_pendingIncrementalUpdates.push(std::move(incrementalUpdate));
      return true;
    }
    if (incrementalUpdate.sequenceStart > (m_sequence + 1)) {
      LOG_WARN("Invalid sequenceStart " << incrementalUpdate.sequenceStart
                                        << " received, existing sequenceEnd is "
                                        << m_sequence);
      return false;
    }
    if (incrementalUpdate.sequenceEnd <= m_sequence) {
      LOG_WARN("Invalid sequenceEnd " << incrementalUpdate.sequenceEnd
                                      << " received, existing sequenceEnd is "
                                      << m_sequence);
      return false;
    }
    applyLevels(incrementalUpdate.bids, m_bids);
    applyLevels(incrementalUpdate.asks, m_asks);
    m_lastUpdateTimestamp = incrementalUpdate.timestamp;
    m_sequence = incrementalUpdate.sequenceEnd;
    return true;
  }

  void printTop10() const {
    std::cout << "========================================\n";
    std::cout << "  ******* *******  ASKs ******* ******* \n";
    std::cout << "----------------------------------------\n";
    std::cout << "           Price          |           Size            |      "
                 "    Sequence        \n";
    std::cout << "----------------------------------------\n";
    std::cout << std::fixed;
    for (auto levelIter = m_asks.rbegin(); levelIter != m_asks.rend();
         ++levelIter) {
      std::cout << std::right << std::setw(25) << std::setprecision(12)
                << levelIter->second.price << " | ";
      std::cout << std::right << std::setw(25) << std::setprecision(12)
                << levelIter->second.size << " | ";
      std::cout << std::right << std::setw(16) << levelIter->second.sequence
                << std::endl;
    }
    std::cout << "----------------------------------------\n";

    std::cout << "  ******* *******  BIDs ******* ******* \n";

    std::cout << "----------------------------------------\n";
    std::cout << "           Price          |           Size            |      "
                 "    Sequence        \n";
    std::cout << "----------------------------------------\n";
    std::cout << std::fixed;
    for (auto& levelIter : m_bids) {
      std::cout << std::right << std::setw(25) << std::setprecision(12)
                << levelIter.second.price << " | ";
      std::cout << std::right << std::setw(25) << std::setprecision(12)
                << levelIter.second.size << " | ";
      std::cout << std::right << std::setw(16) << levelIter.second.sequence
                << std::endl;
    }
    std::cout
        << "============================================================\n";
  }
};
