
#include "OrderBookHTTPServer.h"

#include <boost_http_server/boost_http_server.hpp>

OrderBookHTTPServer::OrderBookHTTPServer(GetSnapshotHandler getSnapshotHandler,
                                         std::string_view host,
                                         std::string_view port,
                                         std::string_view documentDir)
    : m_getSnapshotHandler{getSnapshotHandler},
      m_httpServer{std::make_unique<http::server::server>(
          std::string(host), std::string(port), std::string(documentDir),
          [&](std::string_view uri, const http::server::request& req,
              http::server::reply& rep) { return m_getSnapshotHandler(); })} {}

void OrderBookHTTPServer::run() { m_httpServer->run(); }

OrderBookHTTPServer::~OrderBookHTTPServer() = default;