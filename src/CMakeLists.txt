

add_executable(OrderBook    
    logging.cpp
    OrderBookWsClient.cpp
    OrderBookHTTPClient.cpp
    utils.cpp
    json_utils.cpp
    OrderBookNetworkConnector.cpp
    main.cpp
)

target_link_libraries(OrderBook PRIVATE 
    nlohmann_json::nlohmann_json
    Boost::program_options
)


