/******************************************************************************
 * imu_sim.cpp — 仿真 IMU 数据采集程序
 *
 * 功能：模拟 IMU 传感器数据采集，演示：
 *   1. 生产者-消费者 线程模型
 *   2. 环形缓冲区（无锁队列的简化版）
 *   3. 互斥锁（mutex）保护共享数据
 *   4. 原子变量（atomic）做线程间启停信号
 ******************************************************************************/

// ==================== 头文件包含 ====================

#include <iostream>   // std::cout, std::endl — 控制台输出
#include <thread>     // std::thread — 创建线程
                      // std::this_thread::sleep_for — 线程休眠
#include <array>      // std::array — 固定大小数组（编译期确定长度）
#include <mutex>      // std::mutex — 互斥锁
                      // std::lock_guard — RAII 风格的自动解锁
#include <atomic>     // std::atomic — 原子操作，线程安全读写
#include <chrono>     // std::chrono — 时间库
                      // steady_clock — 单调时钟（不会被系统时间调整影响）
                      // duration_cast — 时间单位转换
                      // milliseconds / seconds / microseconds — 字面量时间单位

// ==================== 数据结构 ====================

/**
 * @brief IMU 数据帧结构体（POD 类型）
 *
 * struct 默认所有成员 public
 * 嵌入式/机器人领域最常用的数据载体，类似 C 语言的 struct
 */
struct ImuFrame
{
    double   ax;             // X 轴加速度，单位 m/s²
    double   ay;             // Y 轴加速度，单位 m/s²
    double   az;             // Z 轴加速度，单位 m/s²（静止时 ≈ 9.81 重力）
    uint64_t timestamp_us;   // 微秒级时间戳（uint64_t 来自 <cstdint>，通过 <iostream> 间接引入）
};

// ==================== 环形缓冲区（Ring Buffer） ====================

/**
 * 环形缓冲区原理：
 *
 *   [0] [1] [2] ... [15]    ← BUF_SIZE=16 个槽位
 *    ↑              ↑
 *   r_ptr          w_ptr    （读指针追写指针）
 *
 * - w_ptr: 写指针，生产者写入下一个位置
 * - r_ptr: 读指针，消费者读取下一个位置
 * - 判满: (w_ptr + 1) % BUF_SIZE == r_ptr  （永远空一个槽，区分"满"和"空"）
 * - 判空: w_ptr == r_ptr
 */
constexpr size_t BUF_SIZE = 16;  // constexpr: 编译期常量，数组大小必须编译期确定

std::array<ImuFrame, BUF_SIZE> ring_buf;  // std::array<元素类型, 元素个数>
                                           // 和 C 数组不同：知道自己的 .size()，不会退化为指针

size_t w_ptr = 0;   // size_t: 无符号整数，专用于表示大小/索引（在 <cstddef> 中定义）
size_t r_ptr = 0;   // 读写指针初始都在 0

std::mutex buf_mtx;  // 保护 ring_buf、w_ptr、r_ptr 的互斥锁
                      // 规则：任何线程访问这三个变量前，必须先 lock 这个 mutex

// ==================== 全局启停信号 ====================

/**
 * std::atomic<bool>: 原子布尔变量
 *
 * 为什么用 atomic 而不是普通 bool？
 *   普通 bool running: 线程 A 写 true，线程 B 可能读到旧值（CPU 缓存不一致）
 *   atomic<bool> running: 保证线程 B 一定能读到线程 A 最新写入的值
 *
 * memory_order_relaxed: 最宽松的内存序，只保证原子性，不保证顺序
 *   这里我们只要"最终能看到"就行，不需要严格的先后顺序，所以用 relaxed 最快
 */
std::atomic<bool> running{true};  // {} 值初始化，等价于 running = true

// ==================== 工具函数 ====================

/**
 * @brief 获取当前单调时钟的微秒时间戳
 *
 * steady_clock: 单调时钟，保证时间只增不减
 *   （system_clock 可能被 NTP 校时或用户手动调整，导致时间倒退）
 *
 * now().time_since_epoch():
 *   返回从时钟纪元（通常是开机时刻）至今的 duration 对象
 *
 * duration_cast<microseconds>(...):
 *   把任意精度的时间差转换为微秒单位
 *
 * .count():
 *   取出 duration 内部的计数值，返回 uint64_t
 *
 * auto: C++11 自动类型推导，编译器根据 = 右边推断变量类型
 *   这里 now 的类型是 std::chrono::steady_clock::time_point
 */
uint64_t get_time_us()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

// ==================== 生产者线程 ====================

/**
 * @brief IMU 数据生产者（模拟传感器采集）
 *
 * 每 10ms 产生一帧模拟 IMU 数据，写入环形缓冲区
 */
void imu_producer()
{
    double noise = 0.0;  // 模拟噪声值（实际应该用随机数）

    // while (running.load(顺序)) :
    //   .load() 是 atomic 的读取方法，比直接读 running 安全
    //   等价于"当 running == true 时循环"
    while (running.load(std::memory_order_relaxed))
    {
        // lock_guard: RAII 风格的锁管理
        //   构造函数: buf_mtx.lock()     ← 获取锁，没拿到就阻塞等待
        //   析构函数: buf_mtx.unlock()   ← 离开 {} 作用域时自动释放
        //   即使循环里 break/continue/抛异常，锁也会被释放，不会死锁
        std::lock_guard<std::mutex> lock(buf_mtx);

        // 判满：写指针的下一个位置 == 读指针 → 缓冲区满了，跳过本次写入
        // % BUF_SIZE: 取模运算，让索引在 0~15 之间循环（即"环形"）
        if ((w_ptr + 1) % BUF_SIZE != r_ptr)
        {
            // 在栈上创建局部变量，离开作用域自动销毁
            ImuFrame frame;
            frame.ax = 0.02 + noise;       // 模拟微小加速度（实际静止时应接近 0）
            frame.ay = -0.01 + noise;
            frame.az = 9.81;               // Z 轴 = 重力加速度
            frame.timestamp_us = get_time_us();

            ring_buf[w_ptr] = frame;        // 写入缓冲区当前写位置
            w_ptr = (w_ptr + 1) % BUF_SIZE; // 写指针前移（环形）
            noise += 0.001;                 // 噪声递增，模拟漂移
        }
        // lock_guard 在这里自动析构，buf_mtx.unlock()

        // this_thread::sleep_for: 让当前线程休眠
        // chrono::milliseconds(10): 字面量，10 毫秒
        // 模拟传感器 100Hz 采样频率（1 秒 / 10ms = 100 次）
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ==================== 消费者线程 ====================

/**
 * @brief IMU 数据消费者（模拟数据处理/显示）
 *
 * 每 15ms 从环形缓冲区取一帧数据打印
 */
void data_consumer()
{
    while (running.load(std::memory_order_relaxed))
    {
        ImuFrame out;         // 用于存放读出的数据
        bool has_data = false; // 标记是否读到了数据

        // 用花括号 {} 创建一个独立作用域
        // 目的：让 lock_guard 在这个作用域结束时释放锁
        // 这样下面的 cout（IO 很慢）不会被锁保护，不阻塞生产者
        {
            std::lock_guard<std::mutex> lock(buf_mtx);

            // 判非空：读指针 != 写指针 → 缓冲区有数据
            if (w_ptr != r_ptr)
            {
                out = ring_buf[r_ptr];               // 从缓冲区读出
                r_ptr = (r_ptr + 1) % BUF_SIZE;      // 读指针前移（环形）
                has_data = true;
            }
        }  // ← lock_guard 在这里析构，自动 unlock，生产者可以继续写

        if (has_data)
        {
            // cout << ... << ... << endl: 链式输出
            // endl 做两件事：输出换行符 '\n' + 刷新缓冲区 (flush)
            // 如果不需要刷新，用 '\n' 更快
            std::cout << "时间戳:" << out.timestamp_us
                      << " ax:" << out.ax
                      << " az:" << out.az << std::endl;
        }

        // 消费者每 15ms 读取一次（慢于生产者的 10ms）
        // 这意味着缓冲区会逐渐积累数据，但只要不溢出就 OK
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

// ==================== 主函数 ====================

/**
 * int main():     程序入口，操作系统从这里开始执行
 * int argc:       命令行参数个数（argument count）
 * char* argv[]:   命令行参数字符串数组（argument vector）
 *   例: ./imu --help  → argc=2, argv[0]="./imu", argv[1]="--help"
 */
int main()
{
    std::cout << "仿真IMU采集程序启动，3秒后自动停止\n";

    // 创建线程对象，构造函数参数是"线程要执行的函数"
    // 构造完成后线程立即开始运行，不等待
    // prod_thread 是线程句柄（handle），用于后续 join
    std::thread prod_thread(imu_producer);   // 启动生产者线程
    std::thread cons_thread(data_consumer);   // 启动消费者线程

    // 主线程休眠 3 秒，让生产者消费者跑一会儿
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 发信号让两个子线程退出
    // .store(value, 内存序): atomic 的写入方法
    running.store(false, std::memory_order_relaxed);

    // join(): 等待线程结束（阻塞调用）
    // 如果不 join，线程对象析构时会调用 std::terminate() 终止整个程序
    // 调用 join 后，线程对象可以被安全析构
    prod_thread.join();
    cons_thread.join();

    std::cout << "程序正常退出\n";
    return 0;  // main 返回 0 表示正常退出，非 0 表示异常
               // 也可以不写 return 0，C++ 规定 main 会自动返回 0
}