#include "OrderBookWsClient.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "AsyncIOHeaders.h"
#include "OrderBook.hpp"
#include "common_header.h"
#include "json_utils.h"
#include "logging.h"

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
  DataCallback m_dataCallback;
  DisconnectCallback m_disconnectCallback;
  std::string m_host;
  std::string m_port;
  std::string m_uri;
  std::string m_text;

  tcp::resolver m_resolver;
  websocket::stream<beast::tcp_stream> m_tcpStream;
  beast::flat_buffer buffer;

 public:
  explicit WebSocketClient(asio::io_context& ioc, DataCallback dataCallback,
                           DisconnectCallback disconnectCallback,
                           std::string_view host, std::string_view port,
                           std::string_view uri)
      : m_dataCallback{dataCallback},
        m_disconnectCallback{disconnectCallback},
        m_host{host},
        m_port{port},
        m_uri{uri},
        m_resolver{asio::make_strand(ioc)},
        m_tcpStream{asio::make_strand(ioc)} {}

  // Start the asynchronous operation
  void run(std::string_view text) {
    // Look up the domain name first
    m_resolver.async_resolve(
        m_host, m_port,
        beast::bind_front_handler(&WebSocketClient::onResolve,
                                  shared_from_this()));
  }

  void onResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
      LOG_ERROR("resolve failed: " << ec.message());
      m_disconnectCallback();
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
      m_disconnectCallback();
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
        websocket::stream_base::decorator([](websocket::request_type& req) {
          req.set(http::field::user_agent,
                  std::string(BOOST_BEAST_VERSION_STRING) +
                      " websocket-client-async");
        }));

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    m_host += ':' + std::to_string(ep.port());
    LOG_INFO("starting handshake m_host: " << m_host);

    // Perform the websocket handshake
    m_tcpStream.async_handshake(
        m_host, m_uri,
        beast::bind_front_handler(&WebSocketClient::onHandshake,
                                  shared_from_this()));
  }

  void onHandshake(beast::error_code ec) {
    LOG_INFO("on handshake, ec: " << ec);
    if (ec) {
      LOG_ERROR("Web socket handshake failed: " << ec.message());
      m_disconnectCallback();
      return;
    }

    // Send the message
    m_tcpStream.async_write(asio::buffer(m_text),
                            beast::bind_front_handler(&WebSocketClient::onWrite,
                                                      shared_from_this()));
  }

  void onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    LOG_INFO("done writing, ec: " << ec);
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      LOG_ERROR("writing data on websocket failed: " << ec.message());
      m_disconnectCallback();
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
      LOG_ERROR("Reading data from websocket failed: " << ec.message());
      m_disconnectCallback();
      return;
    }

    LOG_INFO("Received data on webSocekt, buffer.data().size(): "
             << buffer.data().size() << " "
             << beast::make_printable(buffer.data()) << std::endl);
    m_dataCallback(
        {static_cast<const char*>(buffer.data().data()), buffer.size()});
    buffer.consume(buffer.size());

    m_tcpStream.async_read(buffer,
                           beast::bind_front_handler(&WebSocketClient::onRead,
                                                     shared_from_this()));

    // // Close the WebSocket connection
    // m_tcpStream.async_close(websocket::close_code::normal,
    //                         beast::bind_front_handler(&WebSocketClient::onClose,
    //                                                   shared_from_this()));
  }

  void onClose(beast::error_code ec) {
    LOG_INFO("closed, ec: " << ec);
    if (ec) {
      LOG_ERROR("Closing websocket connection failed: " << ec.message());
      return;
    }
    LOG_INFO("at close buffer size is: " << buffer.data().size());
  }
};

OrderBookWsClient::OrderBookWsClient(
    IncrementalUpdateCallback incrementalUpdateCallback,
    DisconnectCallback disconnectCallback, boost::asio::io_context& ioc,
    std::string_view host, std::string_view port, std::string_view uri)
    : m_host{host},
      m_port{port},
      m_uri{uri},
      m_incrementalUpdateCallback{incrementalUpdateCallback},
      m_disconnectCallback{disconnectCallback},
      m_webSocketClient{std::make_shared<WebSocketClient>(
          ioc,
          [&](std::string_view jsonData) {
            IncrementalUpdate incrementalUpdate;
            jsonToIncrementalUpdate(jsonData, incrementalUpdate);
            m_incrementalUpdateCallback(std::move(incrementalUpdate));
          },
          disconnectCallback, host, port, uri)} {
  m_subscriptionRequestJson =
      "{\n"
      "   \"id\": 1545910660739,\n"
      "   \"type\": \"subscribe\",\n"
      "   \"topic\": \"/market/level2:BTC-USDT\",\n"
      "   \"response\": true\n"
      "}\n";
}

void OrderBookWsClient::run() {
  LOG_INFO("Running OrderBookWsClient");
  m_webSocketClient->run(m_subscriptionRequestJson);
}

void OrderBookWsClient::jsonToIncrementalUpdate(
    std::string_view json, IncrementalUpdate& incrementalUpdate) {
  nlohmann::json jsonIncrementalUpdate = nlohmann::json::parse(json)["data"];
  incrementalUpdate.timestamp =
      TimePoint(jsonIncrementalUpdate["time"].get<long>());
  incrementalUpdate.sequenceStart =
      jsonIncrementalUpdate["sequenceStart"].get<std::size_t>();
  incrementalUpdate.sequenceEnd =
      jsonIncrementalUpdate["sequenceEnd"].get<std::size_t>();
  bool withSequence = true;
  const auto& jsonBids = jsonIncrementalUpdate["changes"]["bids"]
                             .template get<std::vector<nlohmann::json>>();
  JsonUtils::populatePriceLevels(incrementalUpdate.bids, jsonBids,
                                 withSequence);
  const auto& jsonAsks = jsonIncrementalUpdate["changes"]["asks"]
                             .template get<std::vector<nlohmann::json>>();
  JsonUtils::populatePriceLevels(incrementalUpdate.asks, jsonAsks,
                                 withSequence);
}

OrderBookWsClient::~OrderBookWsClient() = default;