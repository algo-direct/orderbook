add_custom_target(order_booker_web_content)

add_custom_command(
    TARGET order_booker_web_content
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/order_booker_web_content"
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Copying assets from ${CMAKE_CURRENT_SOURCE_DIR} to target directory ${CMAKE_BINARY_DIR}/order_booker_web_content"
)