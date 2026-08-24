#include "myCamera.h"
#include <opencv2/opencv.hpp>
#include <iostream>

// 题目1示例：使用 myCamera 类读取相机画面并显示，按 ESC 退出
int main()
{
    myCamera camera(0); // 打开 0 号相机
    cv::Mat frame;

    while (true)
    {
        if (!camera.read(frame))
        {
            std::cerr << "read frame failed!" << std::endl;
            break;
        }
        cv::imshow("myCamera", frame);
        if (cv::waitKey(1) == 27) // ESC
        {
            break;
        }
    }
    return 0;
}
