add_custom_target(integration_tests)

add_custom_command(
    TARGET integration_tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/integration_tests"
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Copying assets from ${CMAKE_CURRENT_SOURCE_DIR} to target directory ${CMAKE_BINARY_DIR}/integration_tests"
)