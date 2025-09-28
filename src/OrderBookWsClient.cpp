#include "OrderBookWsClient.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "OrderBook.hpp"
#include "logging.h"

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

OrderBookWsClient::OrderBookWsClient(std::string_view host,
                                     std::string_view port)
    : m_host(host), m_port(port) {}

void OrderBookWsClient::getSnapshot(OrderBookSnapshot &orderBookSnapshot) {
  net::io_context ioc;
  tcp::resolver resolver(ioc);
  beast::tcp_stream stream(ioc);
  auto const results = resolver.resolve(m_host, m_port);
  stream.connect(results);

  auto target = "/snapshot";
  auto version = 11;
  http::request<http::string_body> req{http::verb::get, target, version};
  req.set(http::field::host, m_host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  http::write(stream, req);

  beast::flat_buffer buffer;
  http::response<http::string_body> httpResult;
  http::read(stream, buffer, httpResult);

  LOG_DEBUG("status: " << httpResult.result_int() << ", content_type: "
                       << (httpResult[http::field::content_type])
                       << ", content_length: "
                       << (httpResult[http::field::content_length]));

  LOG_DEBUG(httpResult);

  if (httpResult.result_int() != 200) {
    throw std::runtime_error("Unable to get snapshot");
  }
  stream.socket().shutdown(tcp::socket::shutdown_both);
  jsonToOrderBookSnapshot(httpResult.body(), orderBookSnapshot);
}

void populatePriceLevels(Levels &levels, const nlohmann::json &jsonLevels,
                         bool withSequence = false) {
  levels.reserve(std::size(jsonLevels));
  for (const auto &jsonLevel : jsonLevels) {
    const auto &bidSizeAndSequence = jsonLevel.get<std::vector<std::string>>();
    if (withSequence) {
      if (size(bidSizeAndSequence) != 3) {
        auto error = std::format(
            "Expect 3 elements in price level json array, but found {}, input "
            "json array '{}'",
            std::size(bidSizeAndSequence), jsonLevel.dump());
        throw std::runtime_error(error);
      }
    } else {
      if (size(bidSizeAndSequence) != 2) {
        auto error = std::format(
            "Expect 2 elements in price level json array, but found {}, input "
            "json array '{}'",
            std::size(bidSizeAndSequence), jsonLevel.dump());
        throw std::runtime_error(error);
      }
    }
    if (bidSizeAndSequence[0].empty()) {
      auto error = std::format("Bid/Ask string is empty, input json array '{}'",
                               jsonLevel.dump());
      throw std::runtime_error(error);
    }
    if (bidSizeAndSequence[1].empty()) {
      auto error = std::format("Size string is empty, input json array '{}'",
                               jsonLevel.dump());
      throw std::runtime_error(error);
    }
    if (bidSizeAndSequence[2].empty()) {
      auto error = std::format(
          "Sequence string is empty, input json array '{}'", jsonLevel.dump());
      throw std::runtime_error(error);
    }
    levels.push_back({.price = std::stod(bidSizeAndSequence[0]),
                      .size = std::stod(bidSizeAndSequence[1])});

    if (withSequence) {
      levels.back().sequence = std::stoull(bidSizeAndSequence[2]);
    }

    if (sizeCompareLessThan(levels.back().size, 0)) {
      auto error = std::format("Size {} is less than 0", bidSizeAndSequence[0]);
      throw std::runtime_error(error);
    }
    if (priceCompareLessThan(levels.back().price, 0)) {
      auto error =
          std::format("Price {} is less than 0", bidSizeAndSequence[0]);
      throw std::runtime_error(error);
    }
  }
}

void OrderBookWsClient::jsonToOrderBookSnapshot(
    std::string_view json, OrderBookSnapshot &orderBookSnapshot) {
  nlohmann::json jsonSnapshot = nlohmann::json::parse(json)["data"];
  orderBookSnapshot.timestamp = TimePoint(jsonSnapshot["time"].get<long>());
  orderBookSnapshot.sequence =
      std::stoull(jsonSnapshot["sequence"].get<std::string>());
  const auto &jsonBids =
      jsonSnapshot["bids"].template get<std::vector<nlohmann::json>>();
  populatePriceLevels(orderBookSnapshot.bids, jsonBids);
  const auto &jsonAsks =
      jsonSnapshot["asks"].template get<std::vector<nlohmann::json>>();
  populatePriceLevels(orderBookSnapshot.asks, jsonAsks);
}

void OrderBookWsClient::jsonToIncrementalUpdate(
    std::string_view json, IncrementalUpdate &incrementalUpdate) {
  nlohmann::json jsonIncrementalUpdate = nlohmann::json::parse(json)["data"];
  incrementalUpdate.timestamp =
      TimePoint(jsonIncrementalUpdate["time"].get<long>());
  incrementalUpdate.sequenceStart =
      jsonIncrementalUpdate["sequenceStart"].get<std::size_t>();
  incrementalUpdate.sequenceStart =
      jsonIncrementalUpdate["sequenceStart"].get<std::size_t>();
  bool withSequence = true;
  const auto &jsonBids = jsonIncrementalUpdate["changes"]["bids"]
                             .template get<std::vector<nlohmann::json>>();
  populatePriceLevels(incrementalUpdate.bids, jsonBids, withSequence);
  const auto &jsonAsks = jsonIncrementalUpdate["changes"]["asks"]
                             .template get<std::vector<nlohmann::json>>();
  populatePriceLevels(incrementalUpdate.asks, jsonAsks, withSequence);
}

void OrderBookWsClient::getIncrementalUpdate(
    IncrementalUpdate &incrementalUpdate) {
  net::io_context ioc;
  tcp::resolver resolver(ioc);
  beast::tcp_stream stream(ioc);
  // auto loopback = "127.0.0.1";
  auto const results = resolver.resolve(m_host, m_port);
  stream.connect(results);

  auto target = "/generateIncrement";
  auto version = 11;
  http::request<http::string_body> req{http::verb::get, target, version};
  req.set(http::field::host, m_host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  http::write(stream, req);

  beast::flat_buffer buffer;
  http::response<http::string_body> httpResult;
  http::read(stream, buffer, httpResult);

  LOG_DEBUG("status: " << httpResult.result_int() << ", content_type: "
                       << (httpResult[http::field::content_type])
                       << ", content_length: "
                       << (httpResult[http::field::content_length]));

  LOG_DEBUG(httpResult);

  if (httpResult.result_int() != 200) {
    throw std::runtime_error("Unable to get snapshot");
  }
  stream.socket().shutdown(tcp::socket::shutdown_both);

  jsonToIncrementalUpdate(httpResult.body(), incrementalUpdate);
}