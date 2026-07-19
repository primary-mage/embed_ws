# ESP32-C3 USB串口/JTAG双向通信示例（精简核心版）
适配：ESP-IDF v5.2.6
硬件：ESP32-C3 板载USB Serial/JTAG
语言：C++
核心能力：USB二进制帧通信、1Hz主动上报、请求应答双模式、CRC校验分包解析

## 1 工程目录（最简）
```
esp32c3_usb_comm/
├── CMakeLists.txt
├── sdkconfig.defaults    # 关闭冗余打印，启用USB-JTAG串口
├── README.md
└── main/
    ├── CMakeLists.txt
    └── app_main.cpp     # 全部业务逻辑
```

## 2 程序启动流程
1. 初始化NVS，持久化记录上电次数`boot_count`
2. 初始化`usb_serial_jtag`驱动
3. 创建两个FreeRTOS常驻任务
   - `push_task`：1Hz周期推送设备运行时间
   - `rx_task`：循环收流、帧解析、CRC校验、命令处理、应答回复
4. 统一发送函数`send_frame()`加互斥锁，防止收发并发串包

## 3 通信协议（核心二进制帧）
### 基础参数
- 物理层：USB Serial/JTAG（Linux：/dev/ttyACM0）
- 帧头：固定同步头 `0xAA 0x55`
- 协议版本：0x01
- 字节序：多字节数值**小端存储**
- 校验：CRC16-CCITT-FALSE（覆盖VER至PAYLOAD）
- 最大负载：32字节，单帧总长最大42字节
- 单帧固定头长：10字节，总长度 = 10 + 负载长度LEN

### 统一帧结构
| 偏移 | 字段     | 长度 | 说明                     |
|------|----------|------|--------------------------|
| 0    | SOF0     | 1B   | 0xAA                     |
| 1    | SOF1     | 1B   | 0x55                     |
| 2    | VER      | 1B   | 协议版本0x01             |
| 3    | TYPE     | 1B   | 指令类型                 |
| 4    | SEQ      | 2B   | 帧序号，请求应答配对用   |
| 6    | LEN      | 2B   | 负载字节长度             |
| 8    | PAYLOAD  | LEN  | 业务数据                 |
| 8+LEN| CRC16    | 2B   | 校验值，小端             |

### 指令TYPE定义（仅保留业务必需）
| TYPE  | 方向         | 帧名               | 功能                     |
|-------|--------------|--------------------|--------------------------|
| 0x01  | 上位机→设备  | GET_UPTIME_REQ     | 查询设备运行时长         |
| 0x10  | 设备→上位机  | UPTIME_PUSH        | 1Hz自动推送设备状态      |
| 0x81  | 设备→上位机  | GET_UPTIME_RSP     | 查询指令应答             |
| 0xE0  | 设备→上位机  | ERROR_RSP          | 非法指令/长度错误应答    |

### 负载格式
1. **查询请求 GET_UPTIME_REQ(0x01)**
LEN=0，无负载；设备收到立刻返回同SEQ的应答帧
2. **推送/应答共用负载（UPTIME_PUSH / GET_UPTIME_RSP，LEN=16）**
| 偏移 | 类型       | 含义                     |
|------|------------|--------------------------|
| 0    | uint64_t   | uptime_ms 上电总毫秒数   |
| 8    | uint32_t   | boot_count NVS上电计数  |
| 12   | uint32_t   | status_flags 设备状态位 |
status_flags Bit0：1=USB已连接主机

3. **错误帧 ERROR_RSP(0xE0)，LEN=4**
| 偏移 | 类型     | 含义               |
|------|----------|--------------------|
| 0    | uint8_t  | error_code错误码   |
| 1    | uint8_t  | offending_type错误指令 |
| 2    | uint16_t | 保留位，固定0      |
错误码：0x01未知指令；0x02负载长度非法

### CRC规则
参与校验区间：VER ~ PAYLOAD；参数：初值0xFFFF，多项式0x1021，输入输出不反转

## 4 两种通信交互逻辑
### 模式1：设备主动推送（1Hz）
设备每秒自动发送UPTIME_PUSH，SEQ本地自增，无需上位机下发指令
### 模式2：请求-应答
上位机下发GET_UPTIME_REQ → 设备校验合法 → 返回带相同SEQ的GET_UPTIME_RSP；参数非法返回ERROR_RSP

## 5 下位机核心函数清单
1. `crc16_ccitt()` CRC校验计算
2. `send_frame()` 带互斥锁统一发送接口
3. `send_device_time_frame()` 组装时间状态帧并发送
4. `extract_frame()` 字节流同步、分包、CRC校验
5. `handle_frame()` 根据TYPE分发处理指令
6. `push_task()` 1Hz周期推送任务
7. `rx_task()` 循环接收解析任务

## 6 编译烧录命令
```bash
source ./activate-esp-idf.sh
cd esp32c3_usb_comm
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash
```

## 7 Linux上位机调试要点
1. 原始模式打开USB串口
```bash
stty -F /dev/ttyACM0 raw 115200 cs8 -cstopb -parenb -ixon -ixoff -echo
```
2. 十六进制抓包查看二进制帧
```bash
dd if=/dev/ttyACM0 bs=1 count=64 status=none | xxd -g1
```
3. 解析器核心逻辑
- 持续扫描流寻找帧头`AA 55`，丢弃杂字节重同步
- 读取固定头部，校验LEN不超32字节上限
- 收取完整负载+CRC，校验通过后按TYPE分发业务处理
- 兼容半包、粘包、CRC错误、设备复位杂数据场景

## 8 关键注意点
1. USB CDC无实际波特率约束，115200仅为上位机软件兼容参数
2. 设备复位/下载阶段会输出非协议杂字节，上位机必须支持帧头重同步
3. 收发共用发送锁，杜绝推送与应答并发输出造成帧错乱