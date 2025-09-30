cmake_minimum_required(VERSION 3.10.0)
project(OrderBook VERSION 0.1.0 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

# include(FetchContent)

# FetchContent_Declare(
#     nlohmann_json
#     GIT_REPOSITORY "https://github.com/nlohmann/json.git"
# )

# FetchContent_MakeAvailable(nlohmann_json)

find_package(Boost REQUIRED
    COMPONENTS program_options
)


add_subdirectory(order_booker_web_content)
add_subdirectory(third_party)
add_subdirectory(src)

include(CTest)
enable_testing()

