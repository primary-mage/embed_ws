cmake_minimum_required(VERSION 3.16)
project(basic_cpp VERSION 1.0.0 LANGUAGES CXX)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件搜索路径（让编译器找到 include/ 下的文件）
include_directories(include)

# 把 utils.cpp 编成静态库
add_library(basic_utils STATIC
    src/utils.cpp
)

# 主程序
add_executable(main
    src/main.cpp
)

# 主程序链接 utils 库
target_link_libraries(main PRIVATE basic_utils)