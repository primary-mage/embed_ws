cmake_minimum_required(VERSION 3.8)
project(serial_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(time_node src/time_node.cpp)
target_link_libraries(time_node pthread)
