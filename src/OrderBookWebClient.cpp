#include "OrderBookWebClient.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "AsyncIOHeaders.h"
#include "OrderBook.hpp"
#include "json_utils.h"
#include "logging.h"

class HttpGetter : public std::enable_shared_from_this<HttpGetter> {
  tcp::resolver m_resolver;
  beast::tcp_stream m_stream;
  beast::flat_buffer m_buffer;
  http::request<http::empty_body> m_httpRequest;
  http::response<http::string_body> m_httpResponse;

 public:
  // Objects are constructed with a strand to
  // ensure that handlers do not execute concurrently.
  explicit HttpGetter(asio::io_context &ioc)
      : m_resolver(asio::make_strand(ioc)), m_stream(asio::make_strand(ioc)) {}

  // Start the asynchronous operation
  void run(std::string_view host, std::string_view port, std::string_view uri,
           int httpVersion) {
    // Set up an HTTP GET request message
    m_httpRequest.version(httpVersion);
    m_httpRequest.method(http::verb::get);
    m_httpRequest.target(uri);
    m_httpRequest.set(http::field::host, host);
    m_httpRequest.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Look up the domain name
    m_resolver.async_resolve(
        host, port,
        beast::bind_front_handler(&HttpGetter::onResolve, shared_from_this()));
  }

  void onResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
      LOG_ERROR("resolve failed: " << ec.message());
      return;
    }

    // Set a timeout on the operation
    m_stream.expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    m_stream.async_connect(
        results,
        beast::bind_front_handler(&HttpGetter::onConnect, shared_from_this()));
  }

  void onConnect(beast::error_code ec,
                 tcp::resolver::results_type::endpoint_type) {
    if (ec) {
      LOG_ERROR("connect failed: " << ec.message());
      return;
    }

    // Set a timeout on the operation
    m_stream.expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(
        m_stream, m_httpRequest,
        beast::bind_front_handler(&HttpGetter::onWrite, shared_from_this()));
  }

  void onWrite(beast::error_code ec, std::size_t bytesTransferred) {
    boost::ignore_unused(bytesTransferred);
    if (ec) {
      LOG_ERROR("sending HttpRequest failed: " << ec.message());
      return;
    }

    // Receive the HTTP response
    http::async_read(
        m_stream, m_buffer, m_httpResponse,
        beast::bind_front_handler(&HttpGetter::onRead, shared_from_this()));
  }

  void onRead(beast::error_code ec, std::size_t bytesTransferred) {
    boost::ignore_unused(bytesTransferred);

    if (ec) {
      LOG_ERROR("getting HttpResponse failed: " << ec.message());
      return;
    }

    // Write the message to standard out
    std::cout << m_httpResponse << std::endl;

    // Gracefully close the socket
    m_stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes so don't bother reporting it.
    if (ec && ec != beast::errc::not_connected) {
      LOG_ERROR("shutdown Http connection failed: " << ec.message());
    }
  }
};

OrderBookWebClient::OrderBookWebClient(boost::asio::io_context &ioc,
                                       std::string_view host,
                                       std::string_view port,
                                       std::string_view uri, int httpVersion)
    : m_host{host},
      m_port{port},
      m_uri{uri},
      m_httpVersion{httpVersion},
      m_httpGetter{std::make_unique<HttpGetter>(ioc)} {}

void OrderBookWebClient::getSnapshot(OrderBookSnapshot &orderBookSnapshot) {
  asio::io_context ioc;
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

void OrderBookWebClient::jsonToOrderBookSnapshot(
    std::string_view json, OrderBookSnapshot &orderBookSnapshot) {
  nlohmann::json jsonSnapshot = nlohmann::json::parse(json)["data"];
  orderBookSnapshot.timestamp = TimePoint(jsonSnapshot["time"].get<long>());
  orderBookSnapshot.sequence =
      std::stoull(jsonSnapshot["sequence"].get<std::string>());
  const auto &jsonBids =
      jsonSnapshot["bids"].template get<std::vector<nlohmann::json>>();
  JsonUtils::populatePriceLevels(orderBookSnapshot.bids, jsonBids);
  const auto &jsonAsks =
      jsonSnapshot["asks"].template get<std::vector<nlohmann::json>>();
  JsonUtils::populatePriceLevels(orderBookSnapshot.asks, jsonAsks);
}

void OrderBookWebClient::jsonToIncrementalUpdate(
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
  JsonUtils::populatePriceLevels(incrementalUpdate.bids, jsonBids,
                                 withSequence);
  const auto &jsonAsks = jsonIncrementalUpdate["changes"]["asks"]
                             .template get<std::vector<nlohmann::json>>();
  JsonUtils::populatePriceLevels(incrementalUpdate.asks, jsonAsks,
                                 withSequence);
}

void OrderBookWebClient::getIncrementalUpdate(
    IncrementalUpdate &incrementalUpdate) {
  asio::io_context ioc;
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
