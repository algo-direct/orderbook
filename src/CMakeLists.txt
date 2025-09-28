

add_executable(OrderBook    
    logging.cpp
    OrderBookWsClient.cpp
    utils.cpp
    main.cpp
)

target_link_libraries(OrderBook PRIVATE nlohmann_json::nlohmann_json)

