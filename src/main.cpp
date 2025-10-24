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
  return 0;
}