/**
 * @file time_node.cpp
 * @brief 定时向 /dev/ttyACM0 发请求、收数据、打印
 */

#include "serial_proto.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#define FREQUENCY 1000

std::atomic<bool> thread_running{true};
void timer_thread(int fd)
{
    uint16_t seq = 0;
    auto tick = std::chrono::steady_clock::now();
    const auto period = std::chrono::milliseconds((uint)1000 / FREQUENCY);
    while (thread_running)
    {
        serial_send_req(fd, seq);
        std::cout << "[SEND] SEQ=" << seq << std::endl;
        serial_recv(fd);
        ++seq;
        tick += period;
        std::this_thread::sleep_until(tick);
    }
}

int main()
{
    int fd = serial_open("/dev/ttyACM1");
    if (fd < 0)
        return 1;
    std::cout << "[INFO] " << FREQUENCY << " Hz 发送 GET_UPTIME_REQ" << std::endl;
    // lambda捕获fd，传给timer_thread，等价于std::thread t(std::bind(timer_thread, fd));等价于std::thread t(timer_thread, fd);
    std::thread t([fd](){ timer_thread(fd); });
    std::cout << "主线程正在做其他事" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "主线程结束" << std::endl;
    thread_running.store(false);
    t.join();
    close(fd);
    return 0;
}