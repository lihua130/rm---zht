#include "myYolo.h"
#include "../io/myCamera.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

// 题目3：YOLO 装甲板识别（框选 + 跟随）
// 用法：
//   yolo_demo model.onnx              相机实时检测
//   yolo_demo model.onnx test.jpg     对单张图片检测
//   yolo_demo model.onnx test.mp4     对视频检测
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "usage: yolo_demo <model.onnx> [image.jpg|video.mp4]" << std::endl;
        return -1;
    }

    std::string modelPath = argv[1];
    std::string source = (argc > 2) ? argv[2] : "";

    myYolo yolo(modelPath, 0.4f);

    // 单张图片模式（第三参数 save：保存 result.jpg 不弹窗，便于无头验证）
    if (!source.empty() && source.find(".jpg") != std::string::npos)
    {
        cv::Mat frame = cv::imread(source);
        if (frame.empty())
        {
            std::cerr << "cannot read image: " << source << std::endl;
            return -1;
        }
        bool found = yolo.detect(frame);
        std::cout << (found ? "armor detected" : "no armor") << std::endl;
        if (argc > 3 && std::string(argv[3]) == "save")
        {
            cv::imwrite("result.jpg", frame);
            std::cout << "saved result.jpg" << std::endl;
        }
        else
        {
            cv::imshow("yolo", frame);
            cv::waitKey(0);
        }
        return 0;
    }

    // 视频 / 相机模式
    cv::VideoCapture cap;
    myCamera camera(0);
    bool useCamera = source.empty();
    if (!useCamera)
        cap.open(source);

    cv::Mat frame;
    while (true)
    {
        bool ok = useCamera ? camera.read(frame) : cap.read(frame);
        if (!ok)
            break;

        yolo.detect(frame);
        cv::imshow("yolo", frame);
        if (cv::waitKey(useCamera ? 1 : 30) == 27) // ESC 退出
            break;
    }
    return 0;
}
