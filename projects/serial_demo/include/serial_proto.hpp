#pragma once
/**
 * @file serial_proto.hpp
 * @brief 串口 + 协议帧 全部封装，对外只暴露 3 个函数
 *
 *   int  serial_open(const char* device);
 *   void serial_send_req(int fd, uint16_t seq);
 *   void serial_recv(int fd);
 */

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <vector>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// ==================== CRC16-CCITT-FALSE ====================
inline uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (static_cast<uint16_t>(data[i]) << 8);
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

// ==================== 串口打开 ====================
inline int serial_open(const char* device) {
    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) { std::cerr << "[ERROR] 无法打开 " << device << std::endl; return -1; }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = CS8 | CREAD | CLOCAL;
    tty.c_iflag = tty.c_oflag = tty.c_lflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 5;
    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}

// ==================== 协议常量 (内部用) ====================
namespace proto {
    constexpr uint8_t  SOF0 = 0xAA, SOF1 = 0x55, VER = 0x01;
    constexpr uint8_t  REQ = 0x01, PUSH = 0x10, RSP = 0x81, ERR = 0xE0;
    constexpr size_t   MAX_LEN = 32;
}

// ==================== 读单字节 ====================
static inline bool _read_byte(int fd, uint8_t& b) { return read(fd, &b, 1) == 1; }

// ==================== 发送请求 ====================
inline void serial_send_req(int fd, uint16_t seq) {
    uint8_t buf[10] = { proto::SOF0, proto::SOF1, proto::VER, proto::REQ,
                        (uint8_t)(seq & 0xFF), (uint8_t)(seq >> 8), 0, 0 };
    uint16_t crc = crc16_ccitt(buf + 2, 6);
    buf[8] = crc & 0xFF; buf[9] = crc >> 8;
    write(fd, buf, 10);
    tcdrain(fd);
}

// ==================== 接收 RSP 帧并打印 (跳过 PUSH) ====================
inline void serial_recv(int fd) {
    uint8_t b;

    while (true) {
        // 同步 AA 55
        while (_read_byte(fd, b)) { if (b == proto::SOF0 && _read_byte(fd, b) && b == proto::SOF1) break; }

        // 读头部 6 字节
        uint8_t hdr[6];
        for (auto& v : hdr) _read_byte(fd, v);

        uint8_t  ver = hdr[0], type = hdr[1];
        uint16_t seq = hdr[2] | (hdr[3] << 8), len = hdr[4] | (hdr[5] << 8);
        if (len > proto::MAX_LEN) continue;

        // 读负载 + CRC
        std::vector<uint8_t> payload(len);
        for (auto& v : payload) _read_byte(fd, v);
        uint8_t crc_buf[2]; _read_byte(fd, crc_buf[0]); _read_byte(fd, crc_buf[1]);
        uint16_t rcv_crc = crc_buf[0] | (crc_buf[1] << 8);

        // CRC 校验
        std::vector<uint8_t> crc_in(6 + len);
        memcpy(crc_in.data(), hdr, 6);
        memcpy(crc_in.data() + 6, payload.data(), len);
        if (rcv_crc != crc16_ccitt(crc_in.data(), crc_in.size())) continue;

        // 跳过 PUSH，只处理 RSP / ERR
        if (type == proto::PUSH) continue;

        // 打印
        std::cout << "[RECV] TYPE=0x" << std::hex << (int)type << std::dec << " SEQ=" << seq;

        if (type == proto::RSP && len >= 16) {
            uint64_t ms; uint32_t boot, flags;
            memcpy(&ms, payload.data(), 8);
            memcpy(&boot, payload.data() + 8, 4);
            memcpy(&flags, payload.data() + 12, 4);
            std::cout << " uptime_ms=" << ms << " boot=" << boot
                      << " usb=" << (flags & 1);
        } else if (type == proto::ERR && len >= 4) {
            std::cout << " err=0x" << std::hex << (int)payload[0] << std::dec;
        }
        std::cout << std::endl;
        return;  // 只处理一帧就返回
    }
}
