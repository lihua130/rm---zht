#ifndef MYCAMERA_H
#define MYCAMERA_H

#include <opencv2/opencv.hpp>

// myCamera：跨平台相机封装类，Windows / Ubuntu 自适应运行
// 公有成员仅包含：构造函数、析构函数、read 函数
class myCamera
{
public:
    // 构造函数：打开相机并设置分辨率
    myCamera(int index = 0, int width = 1280, int height = 720);

    // 析构函数：释放相机资源
    ~myCamera();

    // read 函数：读取一帧图像，成功返回 true
    bool read(cv::Mat &frame);

private:
    cv::VideoCapture cap_; // 相机句柄
    int index_;            // 相机编号
    int width_;            // 画面宽度
    int height_;           // 画面高度
    bool opened_;          // 相机是否打开成功
};

#endif // MYCAMERA_H
