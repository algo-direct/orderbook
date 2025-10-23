#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <thread>

#include "OrderBookHTTPServer.h"
#include "OrderBookNetworkConnector.h"
#include "logging.h"
#include "utils.h"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()                    //
        ("help", "produce help message")  //
        ("host", po::value<std::string>()->default_value("127.0.0.1"),
         "Hostname of OrderBook Feed Server or Simulation")  //
        ("port", po::value<std::string>()->default_value("40000"),
         "Port nuumber of OrderBook Feed Server or Simulation")  //
        ("reconnect_delay", po::value<int>()->default_value(2000),
         "Delay in milliseconds before reconnect to OrderBook Feed Server or "
         "Simulation "
         "after having connection issues.")  //
        ("http_server_host", po::value<std::string>()->default_value("0.0.0.0"),
         "Optional, use as http server host to server OrderBook, "
         "default is 0.0.0.0")  //
        ("http_server_port", po::value<int>()->default_value(48080),
         "Optional, use as http server port to server OrderBook, "
         "if 0 value is provided then it will not start as http server.")  //
        ("http_doc_dir",
         po::value<std::string>()->default_value("../order_booker_web_content"),
         "Top dir path to be used for serveing files over http.");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }

    auto reconnectDelay = vm["reconnect_delay"].as<int>();
    if (reconnectDelay < 10) {
      LOG_ERROR(std::format(
          "reconnect_delay is {}, Cannot reconnect sooner than 10 milliseconds",
          reconnectDelay));
      std::cout << desc << std::endl;
      return 1;
    }

    auto httpServerPort = vm["http_server_port"].as<int>();
    if (httpServerPort < 0) {
      LOG_ERROR(
          std::format("http_server_port is {}, must be a positive port number",
                      httpServerPort));
      std::cout << desc << std::endl;
      return 1;
    }

    if (httpServerPort > 65535) {
      LOG_ERROR(std::format(
          "http_server_port is {}, port number must be less than 65535",
          httpServerPort));
      std::cout << desc << std::endl;
      return 1;
    }

    auto host = vm["host"].as<std::string>();
    auto port = vm["port"].as<std::string>();
    auto httpDocDir = vm["http_doc_dir"].as<std::string>();

    bool runAsHTTPServer = httpServerPort > 0;
    bool useLock = runAsHTTPServer;

    std::filesystem::path currentPath = std::filesystem::current_path();
    // Print the current working directory
    LOG_INFO("Running from working directory: " << currentPath);

    OrderBookNetworkConnector orderBookNetworkConnector(
        host, port, reconnectDelay, useLock);

    std::jthread httpServerThread;  // not started yet
    std::unique_ptr<OrderBookHTTPServer> m_orderBookHTTPServer;
    if (runAsHTTPServer) {
      m_orderBookHTTPServer = std::make_unique<OrderBookHTTPServer>(
          [&]() {
            // callback hit by http server to get snapshot
            try {
              return orderBookNetworkConnector.getSnapshot();
            } catch (const std::exception& exp) {
              LOG_ERROR(
                  "Exception occured while generating snapshot for http "
                  "server: "
                  << exp.what());
              throw;
            }
          },
          vm["http_server_host"].as<std::string>(),
          std::to_string(httpServerPort), httpDocDir);
      httpServerThread = std::jthread([&]() { m_orderBookHTTPServer->run(); });
    }
    orderBookNetworkConnector.run();
  } catch (const std::exception& exp) {
    std::cout << "exception occured: " << exp.what() << std::endl;
    return 1;
  } catch (...) {
    std::cout << "unknown exception occured: " << std::endl;
    return 1;
  }

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