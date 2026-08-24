#include "myComm.h"
#include "../io/myCamera.h"
#include "../task2/myArmor.h"
#include <opencv2/opencv.hpp>
#include <iostream>

// 题目4：识别装甲板 -> 将像素坐标下传单片机（上下位机通信）
// 帧格式：0xAA + x(uint16 LE) + y(uint16 LE) + 0xBB（画面左上角为 0,0）
// 用法：
//   comm_demo COM5 red    识别红色装甲板，坐标下传到 COM5（Linux 用 /dev/ttyUSB0）
//   comm_demo none blue   空转模式：不开发串口，只打印（无硬件调试）
int main(int argc, char **argv)
{
    std::string port = (argc > 1) ? argv[1] : "none";
    int color = (argc > 2 && std::string(argv[2]) == "blue") ? myArmor::BLUE
                                                             : myArmor::RED;

    myCamera camera(0);
    myArmor armor(color);
    myComm comm(port);

    cv::Mat frame;
    while (camera.read(frame))
    {
        if (armor.detect(frame))
        {
            cv::Point2f t = armor.target();
            comm.send((int)t.x, (int)t.y); // 下传装甲板中心像素坐标
        }
        cv::imshow("comm", frame);
        if (cv::waitKey(1) == 27) // ESC 退出
            break;
    }
    return 0;
}
