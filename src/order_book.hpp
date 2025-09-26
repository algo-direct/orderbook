#include <cstddef>
#include <vector>
#include <queue>
#include <chrono>
#include <map>
#include <cmath>
#include <limits>
#include <algorithm>

#include "logging.h"

using PriceType = double;
using SizeType = double;
using SequenceType = std::size_t;
using TimePoint = std::chrono::milliseconds;

struct Level
{
    PriceType price{0};
    SizeType size{0};
    SequenceType sequence{0};
};

const double PRICE_COMPARISION_TOLERANCE = 1e-12;
const double SIZE_COMPARISION_TOLERANCE = 1e-12;

bool priceCompareEqual(const PriceType &lhs, const PriceType &rhs)
{
    if (std::abs(lhs - rhs) < PRICE_COMPARISION_TOLERANCE)
    {
        return true;
    }
    return false;
}

bool sizeCompareEqual(const SizeType &lhs, const SizeType &rhs)
{
    if (std::abs(lhs - rhs) < SIZE_COMPARISION_TOLERANCE)
    {
        return true;
    }
    return false;
}

struct PriceCompareLessThan
{
    bool operator()(const PriceType &lhs, const PriceType &rhs) const
    {
        if (priceCompareEqual(lhs, rhs))
        {
            return false;
        }
        return lhs < rhs;
    }
};

struct PriceCompareGreaterThan
{
    bool operator()(const PriceType &lhs, const PriceType &rhs) const
    {
        if (priceCompareEqual(lhs, rhs))
        {
            return false;
        }
        return lhs > rhs;
    }
};

using Levels = std::vector<Level>;
using AskLevels = std::map<PriceType, Level, PriceCompareLessThan>;
using BidLevels = std::map<PriceType, Level, PriceCompareGreaterThan>;

enum struct BidOrAsk
{
    BID,
    ASK
};

struct OrderBookSnapshot
{
    SequenceType sequence;
    TimePoint timestamp;
    Levels bids;
    Levels asks;
};

struct IncrementalUpdate
{
    SequenceType sequenceStart{0};
    SequenceType sequenceEnd{0};
    TimePoint timestamp;
    Levels bids;
    Levels asks;
};

class OrderBook
{
    SequenceType sequence{0};
    TimePoint lastUpdateTimestamp;
    BidLevels bids;
    AskLevels asks;
    std::queue<IncrementalUpdate> pendingIncrementalUpdates;

public:
    void applySnapshot(OrderBookSnapshot& orderBookSnapshot) {
        lastUpdateTimestamp = orderBookSnapshot.timestamp;
        sequence = orderBookSnapshot.sequence;
        for (auto &level : orderBookSnapshot.bids)
        {
            level.sequence = sequence;
            bids[level.price] = level;
        }
        for (auto &level : orderBookSnapshot.asks)
        {
            level.sequence = sequence;
            asks[level.price] = level;
        }

        while(!pendingIncrementalUpdates.empty()) {
            applyIncrementalUpdate(std::move(pendingIncrementalUpdates.front()));
            pendingIncrementalUpdates.pop();
        }
    }

    template <typename LevelType>
    void applyLevels(const Levels &inputLevels, LevelType &levels)
    {
        for (auto &inputLevel : inputLevels)
        {
            if (inputLevel.sequence <= sequence)
            {
                continue;
            }
            if (sizeCompareEqual(inputLevel.size, 0))
            {
                levels.erase(inputLevel.price);
                continue;
            }
            auto insertResult = levels.insert({inputLevel.price, inputLevel});
            if (!insertResult.second)
            {
                auto &level = insertResult.first->second;
                if (inputLevel.sequence <= level.sequence)
                {
                    continue;
                }
                //  If the price is 0, ignore the messages and update the sequence.
                if (priceCompareEqual(inputLevel.price, 0))
                {
                    level.sequence = inputLevel.sequence;
                    continue;
                }
                level.sequence = inputLevel.sequence;
                level.size = inputLevel.size;
                level.price = inputLevel.price;
            }
        }
    }

    bool applyIncrementalUpdate(IncrementalUpdate&& incrementalUpdate)
    {
        if(sequence == 0) {
            // snapshot not received yet
            pendingIncrementalUpdates.push(std::move(incrementalUpdate));
            return true;
        }
        if (incrementalUpdate.sequenceStart > (sequence + 1))
        {
            LOG_WARN << "Invalid sequenceStart " << incrementalUpdate.sequenceStart << " received, existing sequenceEnd is " << sequence;
            return false;
        }
        if (incrementalUpdate.sequenceEnd <= sequence)
        {
            LOG_WARN << "Invalid sequenceEnd " << incrementalUpdate.sequenceEnd << " received, existing sequenceEnd is " << sequence;
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
        std::cout << "           Price          |           Size            |          Sequence        \n";
        std::cout << "----------------------------------------\n";
        std::cout << std::fixed;
        for(auto levelIter = asks.rbegin(); levelIter != asks.rend(); ++levelIter) {
            std::cout << std::right << std::setw(25) << std::setprecision(12) << levelIter->second.price  << " | ";
            std::cout << std::right << std::setw(25) << std::setprecision(12) << levelIter->second.size  << " | ";            
            std::cout << std::right << std::setw(16) << levelIter->second.sequence  << std::endl;
        }
        std::cout << "----------------------------------------\n";
        
        std::cout << "  ******* *******  BIDs ******* ******* \n";

        std::cout << "----------------------------------------\n";
        std::cout << "           Price          |           Size            |          Sequence        \n";
        std::cout << "----------------------------------------\n";
        std::cout << std::fixed;
        for(auto& levelIter: bids) {
            std::cout << std::right << std::setw(25) << std::setprecision(12) << levelIter.second.price  << " | ";
            std::cout << std::right << std::setw(25) << std::setprecision(12) << levelIter.second.size  << " | ";            
            std::cout << std::right << std::setw(16) << levelIter.second.sequence  << std::endl;
        }
        std::cout << "============================================================\n";
    }
};