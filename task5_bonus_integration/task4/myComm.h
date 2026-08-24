#ifndef MYCOMM_H
#define MYCOMM_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// myComm：电脑-单片机串口通信类（电脑端）
// 协议：帧头 0xAA + 数据 + 帧尾 0xBB
// 数据格式：x(uint16 小端) + y(uint16 小端)，画面左上角为 (0,0)
// 一帧共 6 字节：[0xAA][xL][xH][yL][yH][0xBB]
class myComm
{
public:
    // 构造函数：port 串口名（Windows 如 "COM5"，Linux 如 "/dev/ttyUSB0"），baud 波特率
    // port 传 "none" 时为空转模式：不打开串口，仅打印待发送数据（调试用）
    explicit myComm(const std::string &port, int baud = 115200);
    ~myComm();

    // 下传一组像素坐标，返回是否发送成功
    bool send(int x, int y);

private:
#ifdef _WIN32
    HANDLE handle_; // Windows 串口句柄
#else
    int fd_;        // Linux 串口文件描述符
#endif
    bool opened_;   // 串口是否打开成功
    bool dummy_;    // 空转模式（无硬件调试）
};

#endif // MYCOMM_H
