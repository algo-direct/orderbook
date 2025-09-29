add_library(boost_http_server STATIC
    connection.cpp
    connection_manager.cpp
    mime_types.cpp
    reply.cpp
    request_handler.cpp
    request_parser.cpp
    server.cpp
)

target_include_directories(boost_http_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)