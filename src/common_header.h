#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <queue>
#include <vector>

const double PRICE_COMPARISION_TOLERANCE = 1e-12;
const double SIZE_COMPARISION_TOLERANCE = 1e-12;

using PriceType = double;
using SizeType = double;
using SequenceType = std::size_t;
using TimePoint = std::chrono::milliseconds;

struct Level {
  PriceType price{0};
  SizeType size{0};
  SequenceType sequence{0};
};

using Levels = std::vector<Level>;

struct PriceCompareLessThan {
  bool operator()(const PriceType &lhs, const PriceType &rhs) const;
};

struct PriceCompareGreaterThan {
  bool operator()(const PriceType &lhs, const PriceType &rhs) const;
};

using AskLevels = std::map<PriceType, Level, PriceCompareLessThan>;
using BidLevels = std::map<PriceType, Level, PriceCompareGreaterThan>;

enum struct BidOrAsk { BID, ASK };

struct OrderBookSnapshot {
  SequenceType sequence;
  TimePoint timestamp;
  Levels bids;
  Levels asks;
};

struct IncrementalUpdate {
  SequenceType sequenceStart{0};
  SequenceType sequenceEnd{0};
  TimePoint timestamp;
  Levels bids;
  Levels asks;
};

using DataCallback = std::function<void(std::string_view)>;