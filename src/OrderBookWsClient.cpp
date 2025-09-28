#include "OrderBookWsClient.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "AsyncIOHeaders.h"
#include "OrderBook.hpp"
#include "common_header.h"
#include "json_utils.h"
#include "logging.h"

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
  tcp::resolver m_resolver;
  websocket::stream<beast::tcp_stream> m_tcpStream;
  beast::flat_buffer buffer;
  std::string m_host;
  std::string m_text;

 public:
  // Resolver and socket require an io_context
  explicit WebSocketClient(asio::io_context &ioc)
      : m_resolver(asio::make_strand(ioc)),
        m_tcpStream(asio::make_strand(ioc)) {}

  // Start the asynchronous operation
  void run(char const *host, char const *port, char const *text) {
    // Save these for later
    host = host;
    m_text = text;

    // Look up the domain name
    m_resolver.async_resolve(
        host, port,
        beast::bind_front_handler(&WebSocketClient::onResolve,
                                  shared_from_this()));
  }

  void onResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
      LOG_ERROR("resolve failed: " << ec.message());
      return;
    }

    // Set the timeout for the operation
    beast::get_lowest_layer(m_tcpStream)
        .expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(m_tcpStream)
        .async_connect(results,
                       beast::bind_front_handler(&WebSocketClient::onConnect,
                                                 shared_from_this()));
  }

  void onConnect(beast::error_code ec,
                 tcp::resolver::results_type::endpoint_type ep) {
    if (ec) {
      LOG_ERROR("connect failed: " << ec.message());
      return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(m_tcpStream).expires_never();

    // Set suggested timeout settings for the websocket
    m_tcpStream.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    m_tcpStream.set_option(
        websocket::stream_base::decorator([](websocket::request_type &req) {
          req.set(http::field::user_agent,
                  std::string(BOOST_BEAST_VERSION_STRING) +
                      " websocket-client-async");
        }));

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    m_host += ':' + std::to_string(ep.port());

    // Perform the websocket handshake
    m_tcpStream.async_handshake(
        m_host, "/",
        beast::bind_front_handler(&WebSocketClient::onHandshake,
                                  shared_from_this()));
  }

  void onHandshake(beast::error_code ec) {
    if (ec) {
      LOG_ERROR("Web socket handshake failed: " << ec.message());
      return;
    }

    // Send the message
    m_tcpStream.async_write(asio::buffer(m_text),
                            beast::bind_front_handler(&WebSocketClient::onWrite,
                                                      shared_from_this()));
  }

  void onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      LOG_ERROR("writing data on websocket failed: " << ec.message());
      return;
    }

    // Read a message into our buffer
    m_tcpStream.async_read(buffer,
                           beast::bind_front_handler(&WebSocketClient::onRead,
                                                     shared_from_this()));
  }

  void onRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      LOG_ERROR("reading data from websocket failed: " << ec.message());
      return;
    }

    // Close the WebSocket connection
    m_tcpStream.async_close(websocket::close_code::normal,
                            beast::bind_front_handler(&WebSocketClient::onClose,
                                                      shared_from_this()));
  }

  void onClose(beast::error_code ec) {
    if (ec) {
      LOG_ERROR("Closing websocket connection failed: " << ec.message());
      return;
    }

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    std::cout << beast::make_printable(buffer.data()) << std::endl;
  }
};

OrderBookWsClient::OrderBookWsClient(boost::asio::io_context &ioc,
                                     std::string_view host,
                                     std::string_view port,
                                     std::string_view uri, int httpVersion)
    : m_host{host},
      m_port{port},
      m_uri{uri},
      m_httpVersion{httpVersion},
      m_webSocketClient{std::make_unique<WebSocketClient>(ioc)} {}

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
  JsonUtils::populatePriceLevels(incrementalUpdate.bids, jsonBids,
                                 withSequence);
  const auto &jsonAsks = jsonIncrementalUpdate["changes"]["asks"]
                             .template get<std::vector<nlohmann::json>>();
  JsonUtils::populatePriceLevels(incrementalUpdate.asks, jsonAsks,
                                 withSequence);
}

void OrderBookWsClient::sendSubscriptionRequest() {}