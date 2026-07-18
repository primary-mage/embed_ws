cmake_minimum_required(VERSION 3.10)
project(practice_cpp)

set(CMAKE_CXX_STANDARD 17)

# 添加可执行文件
add_executable(hello_cpp src/hello.cpp)
add_executable(imu src/imu_sim.cpp)
add_executable(class src/class.cpp)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)


# 添加链接库
target_link_libraries(hello_cpp pthread)#这里的名称需要和可执行文件保持一致
target_link_libraries(imu pthread)