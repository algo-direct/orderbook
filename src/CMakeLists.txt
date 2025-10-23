

add_executable(OrderBook    
    logging.cpp
    OrderBookWsClient.cpp
    OrderBookHTTPClient.cpp
    OrderBookHTTPServer.cpp
    utils.cpp
    json_utils.cpp
    OrderBookNetworkConnector.cpp
    main.cpp
)

target_include_directories(OrderBook PRIVATE 
     ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(OrderBook PRIVATE 
    Boost::program_options
    boost_http_server
)

add_dependencies(OrderBook order_booker_web_content integration_tests simulator)
