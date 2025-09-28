
#include "order_book.hpp"
 
int main()
{
    std::cout << "------------------ start" << std::endl;
    // OrderBookSnapshot orderBookSnapshot;
    // orderBookSnapshot.sequence = 16;
    // orderBookSnapshot.asks.reserve(4);
    // orderBookSnapshot.asks.push_back({.price = 3988.62, .size = 8});
    // orderBookSnapshot.asks.push_back({.price = 3988.61, .size = 32});
    // orderBookSnapshot.asks.push_back({.price = 3988.60, .size = 47});
    // orderBookSnapshot.asks.push_back({.price = 3988.59, .size = 3});

    // orderBookSnapshot.bids.reserve(4);
    // orderBookSnapshot.bids.push_back({.price = 3988.48, .size = 10});
    // orderBookSnapshot.bids.push_back({.price = 3988.49, .size = 100});
    // orderBookSnapshot.bids.push_back({.price = 3988.50, .size = 15});
    // orderBookSnapshot.bids.push_back({.price = 3988.51, .size = 56});

    // OrderBook orderBook;
    // orderBook.applySnapshot(orderBookSnapshot);
    // orderBook.printTop10();

    // IncrementalUpdate incrementalUpdate;
    // incrementalUpdate.sequenceStart = 15;
    // incrementalUpdate.sequenceEnd = 19;
    // incrementalUpdate.asks.reserve(3);
    // incrementalUpdate.asks.push_back({.price = 3988.59, .size = 3, .sequence = 16});
    // incrementalUpdate.asks.push_back({.price = 3988.61, .size = 0, .sequence = 19});
    // incrementalUpdate.asks.push_back({.price = 3988.62, .size = 8, .sequence = 15});

    // incrementalUpdate.bids.push_back({.price = 3988.50, .size = 44, .sequence = 18});
    // orderBook.applyIncrementalUpdate(std::move(incrementalUpdate));
    // orderBook.printTop10();
    std::cout << "------------------ end" << std::endl;
    return 0;
}