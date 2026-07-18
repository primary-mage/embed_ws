#include <iostream>
#include <vector>
using namespace std;

// 电机类（机器人关节电机）
class Motor
{
public:
    int id;         // 电机编号
    int speed;      // 当前转速

    // 【构造函数】对象创建时自动调用，初始化电机
    Motor(int motor_id, int init_speed) : id(motor_id), speed(init_speed)
    {
        cout << "创建电机" << id << "，初始化转速=" << speed << endl;
        // 实际项目这里：打开串口、申请寄存器、硬件初始化
    }

    // 【析构函数】对象销毁时自动调用，清理硬件资源
    ~Motor()
    {
        cout << "销毁电机" << id << "，释放硬件通道" << endl;
        // 实际项目这里：关闭串口、释放IO、回收内存
    }

    // 修改转速
    void set_speed(int s)
    {
        speed = s;
    }
};

int main()
{
    vector<Motor> motors;

    // 批量创建电机，构造函数自动执行初始化
    motors.emplace_back(1, 100);
    motors.emplace_back(2, 200);
    motors.emplace_back(3, 150);

    cout << "===== 使用lambda遍历所有电机，统一调速 =====" << endl;
    // 【Lambda表达式】匿名函数，临时写一段逻辑快速处理容器数据
    auto set_all_slow = [](Motor& m) {
        m.set_speed(50);
        cout << "电机" << m.id << "限速至" << m.speed << endl;
    };

    // 遍历执行lambda
    for (auto& m : motors)
    {
        set_all_slow(m);
    }

    cout << "===== main函数结束，容器销毁，自动触发析构 =====" << endl;
    return 0;
}
