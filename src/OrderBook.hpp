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
  SequenceType sequence{0};
  TimePoint lastUpdateTimestamp;
  BidLevels bids;
  AskLevels asks;
  std::queue<IncrementalUpdate> pendingIncrementalUpdates;

 public:
  void applySnapshot(OrderBookSnapshot &orderBookSnapshot) {
    lastUpdateTimestamp = orderBookSnapshot.timestamp;
    sequence = orderBookSnapshot.sequence;
    for (auto &level : orderBookSnapshot.bids) {
      level.sequence = sequence;
      bids[level.price] = level;
    }
    for (auto &level : orderBookSnapshot.asks) {
      level.sequence = sequence;
      asks[level.price] = level;
    }

    while (!pendingIncrementalUpdates.empty()) {
      applyIncrementalUpdate(std::move(pendingIncrementalUpdates.front()));
      pendingIncrementalUpdates.pop();
    }
  }

  template <typename LevelType>
  void applyLevels(const Levels &inputLevels, LevelType &levels) {
    for (auto &inputLevel : inputLevels) {
      if (inputLevel.sequence <= sequence) {
        continue;
      }
      if (sizeCompareEqual(inputLevel.size, 0)) {
        levels.erase(inputLevel.price);
        continue;
      }
      auto insertResult = levels.insert({inputLevel.price, inputLevel});
      if (!insertResult.second) {
        auto &level = insertResult.first->second;
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

  bool applyIncrementalUpdate(IncrementalUpdate &&incrementalUpdate) {
    if (sequence == 0) {
      // snapshot not received yet
      pendingIncrementalUpdates.push(std::move(incrementalUpdate));
      return true;
    }
    if (incrementalUpdate.sequenceStart > (sequence + 1)) {
      LOG_WARN("Invalid sequenceStart " << incrementalUpdate.sequenceStart
                                        << " received, existing sequenceEnd is "
                                        << sequence);
      return false;
    }
    if (incrementalUpdate.sequenceEnd <= sequence) {
      LOG_WARN("Invalid sequenceEnd " << incrementalUpdate.sequenceEnd
                                      << " received, existing sequenceEnd is "
                                      << sequence);
      return false;
    }
    applyLevels(incrementalUpdate.bids, bids);
    applyLevels(incrementalUpdate.asks, asks);
    lastUpdateTimestamp = incrementalUpdate.timestamp;
    sequence = incrementalUpdate.sequenceEnd;
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
    for (auto levelIter = asks.rbegin(); levelIter != asks.rend();
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
    for (auto &levelIter : bids) {
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