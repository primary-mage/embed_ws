#include <iostream>
#include <thread>
#include <mutex>
    
std::mutex mtx;
int sum = 0;
int func1() {
    std::cout << "func1 is running in a thread." << std::endl;
    std::lock_guard<std::mutex> lock(mtx); // 加锁
    for (int i = 0; i < 100; ++i) {
        sum += i;
    }
    return 0;
}

int func2() {
    std::cout << "func2 is running in a thread." << std::endl;
    std::lock_guard<std::mutex> lock(mtx); // 加锁
    std::cout << sum << std::endl;
    return 0;
}

int main() {
    // 创建线程
    std::thread t1(func1);
    std::thread t2(func2);
    std::cout << "main is over." << std::endl;
    t1.join();
    t2.join();
    return 0;

}

//这个演示程序有多种结果，充分展示了多线程的特性。
//primarymage@primarymage-Vostro-15-5510:/embed_workspace/projects/thread_cpp/build$ ./thread_cpp 
// func1 is running in a thread.
// func2 is running in a thread.
// 4950
// main is over.

// primarymage@primarymage-Vostro-15-5510:/embed_workspace/projects/thread_cpp/build$ ./thread_cpp 
// main is over.
// func2 is running in a thread.
// func1 is running in a thread.
// 0

//创建 ≠ 开始执行，线程创建后，真正的执行顺序由cpu调度决定