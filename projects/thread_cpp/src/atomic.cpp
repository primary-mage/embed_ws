#include <iostream>
#include <atomic>
#include <thread>

std::atomic<int> share_data(0);

void turn_1() {
    share_data.store(1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Thread 1: share_data = " << share_data.load() << std::endl;
}

void turn_2() {
    share_data.store(2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Thread 2: share_data = " << share_data.load() << std::endl;
}

int main() {
    std::thread t1(turn_1);
    std::thread t2(turn_2);

    t1.join();
    t2.join();

    std::cout << "Final value: " << share_data.load() << std::endl;

    return 0;
}
//atomic只保证但行读写,在这里会导致同一个线程中赋值和打印出来的并不相等