#include <boost/program_options.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <ranges>
#include <string>

#include "OrderBookNetworkConnector.h"
#include "logging.h"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "host", po::value<std::string>()->default_value("127.0.0.1"),
      "Hostname of OrderBook Feed Server or Simulation")  //
      ("port", po::value<std::string>()->default_value("40000"),
       "Port nuumber of OrderBook Feed Server or Simulation");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  OrderBookNetworkConnector orderBookNetworkConnector(
      vm["host"].as<std::string>(), vm["port"].as<std::string>());
  orderBookNetworkConnector.run();
  LOG_INFO("After orderBookNetworkConnector.run()\n");

  // asio::io_context ioc;

  // std::cout << "------------------ start" << std::endl;
  // const auto DEFAULT_HTTP_CLIENT = 11;  // HTTP 1.1
  // auto onIncrementalUpdate =
  //     [&orderBook](IncrementalUpdate&& incrementalUpdate) {
  //       if (!orderBook) {
  //         return;
  //       }
  //       orderBook->applyIncrementalUpdate(std::move(incrementalUpdate));
  //     };
  // auto disconnectCallback = [&orderBook]() {
  //   if (!orderBook) {
  //     return;
  //   }
  // };
  // OrderBookWsClient orderBookWsClient(onIncrementalUpdate,
  // disconnectCallback,
  //                                     ioc, vm["host"].as<std::string>(),
  //                                     vm["port"].as<std::string>(), "/ws",
  //                                     DEFAULT_HTTP_CLIENT);
  // OrderBookHTTPClient orderBookHTTPClient(ioc, vm["host"].as<std::string>(),
  //                                       vm["port"].as<std::string>(), "/ws",
  //                                       DEFAULT_HTTP_CLIENT);

  // OrderBookSnapshot orderBookSnapshot;
  // // orderBookWsClient.getSnapshot(orderBookSnapshot);
  // // orderBook.applySnapshot(orderBookSnapshot);
  // // orderBook.printTop10();
  // for (int _ : std::views::iota(0, 10000)) {
  //   // IncrementalUpdate incrementalUpdate;
  //   // orderBookWsClient.getIncrementalUpdate(incrementalUpdate);
  //   // orderBook.printTop10();
  // }

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
  // incrementalUpdate.asks.push_back({.price = 3988.59, .size = 3, .sequence =
  // 16}); incrementalUpdate.asks.push_back({.price = 3988.61, .size = 0,
  // .sequence = 19}); incrementalUpdate.asks.push_back({.price = 3988.62, .size
  // = 8, .sequence = 15});

  // incrementalUpdate.bids.push_back({.price = 3988.50, .size = 44, .sequence =
  // 18}); orderBook.applyIncrementalUpdate(std::move(incrementalUpdate));
  // orderBook.printTop10();
  // ioc.run();
  // std::cout << "------------------ end" << std::endl;

  return 0;
}