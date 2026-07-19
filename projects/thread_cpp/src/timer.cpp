#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono;

void timer_1s_thread(){
    auto tick = steady_clock::now();
    const auto period=milliseconds(1000);
    while(true){
        std::cout << "Timer 1s: Hello, World!" << std::endl;
        tick+=period;
        std::this_thread::sleep_until(tick);
    }
}

void timer_2s_thread(){
    auto tick = steady_clock::now();
    const auto period=milliseconds(2000);
    while(true){
        std::cout << "Timer 2s: Hello, World!" << std::endl;
        tick+=period;
        std::this_thread::sleep_until(tick);
    }
}

int main(){
    std::thread t1(timer_1s_thread);
    std::thread t2(timer_2s_thread);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    t1.detach();
    t2.detach();
    return 0;
}