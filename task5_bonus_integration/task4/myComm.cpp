#include "myComm.h"
#include <cstdint>
#include <iostream>

#ifndef _WIN32
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

myComm::myComm(const std::string &port, int baud)
    : opened_(false), dummy_(port == "none")
{
#ifdef _WIN32
    handle_ = INVALID_HANDLE_VALUE;
#endif
    if (dummy_)
    {
        std::cout << "[myComm] dummy mode, data will only be printed" << std::endl;
        return;
    }

#ifdef _WIN32
    // Windows：打开 COM 口并配置 8N1
    std::string dev = "\\\\.\\" + port;
    handle_ = CreateFileA(dev.c_str(), GENERIC_WRITE, 0, nullptr,
                          OPEN_EXISTING, 0, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[myComm] open " << port << " failed" << std::endl;
        return;
    }
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(handle_, &dcb);
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(handle_, &dcb);
    opened_ = true;
#else
    // Linux：打开 ttyUSB 并配置 8N1 原始模式
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0)
    {
        std::cerr << "[myComm] open " << port << " failed" << std::endl;
        return;
    }
    termios tty{};
    tcgetattr(fd_, &tty);
    speed_t speed = (baud == 115200) ? B115200 : B9600;
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 数据位
    tty.c_cflag &= ~(PARENB | CSTOPB);          // 无校验、1 停止位
    tty.c_lflag = 0;                            // 原始模式
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tcsetattr(fd_, TCSANOW, &tty);
    opened_ = true;
#endif
}

myComm::~myComm()
{
#ifdef _WIN32
    if (opened_)
        CloseHandle(handle_);
#else
    if (opened_)
        ::close(fd_);
#endif
}

bool myComm::send(int x, int y)
{
    // 组帧：0xAA + x(uint16 LE) + y(uint16 LE) + 0xBB
    uint8_t frame[6];
    frame[0] = 0xAA;
    frame[1] = (uint8_t)(x & 0xFF);
    frame[2] = (uint8_t)((x >> 8) & 0xFF);
    frame[3] = (uint8_t)(y & 0xFF);
    frame[4] = (uint8_t)((y >> 8) & 0xFF);
    frame[5] = 0xBB;

    if (dummy_)
    {
        std::cout << "[myComm] send: x=" << x << " y=" << y << std::endl;
        return true;
    }
    if (!opened_)
        return false;

#ifdef _WIN32
    DWORD written = 0;
    return WriteFile(handle_, frame, sizeof(frame), &written, nullptr) && written == sizeof(frame);
#else
    return ::write(fd_, frame, sizeof(frame)) == (ssize_t)sizeof(frame);
#endif
}
