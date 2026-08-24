#include "myArmor.h"
#include "../io/myCamera.h"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>
#include <string>

// 题目2：传统视觉装甲板识别（框选 + 跟随 + 数字识别）
// 用法：
//   armor_demo                 默认红色敌方，相机实时检测
//   armor_demo blue            蓝色敌方，相机实时检测
//   armor_demo red test.jpg    对单张图片检测
//   armor_demo red test.mp4    对视频检测
int main(int argc, char **argv)
{
    int color = myArmor::RED;
    std::string source;

    if (argc > 1)
        color = (std::string(argv[1]) == "blue") ? myArmor::BLUE : myArmor::RED;
    if (argc > 2)
        source = argv[2];

    myArmor armor(color);

    // 单张图片模式（第三个参数为 save 时保存 result.jpg 并直接退出）
    if (!source.empty() && source.find(".jpg") != std::string::npos)
    {
        cv::Mat frame = cv::imread(source);
        if (frame.empty())
        {
            std::cerr << "cannot read image: " << source << std::endl;
            return -1;
        }
        bool found = armor.detect(frame);
        cv::Point2f t = armor.target();
        std::cout << (found ? "armor found" : "no armor")
                  << ", center=(" << t.x << "," << t.y << ")";
        if (found)
            std::cout << ", dist=" << armor.distance() << "m"
                      << ", yaw=" << armor.yaw() << "deg"
                      << ", number=" << armor.number();
        std::cout << std::endl;
        if (argc > 3 && std::string(argv[3]) == "save")
        {
            cv::imwrite("result.jpg", frame);
            std::cout << "result saved to result.jpg" << std::endl;
            return 0;
        }
        cv::imshow("armor", frame);
        cv::waitKey(0);
        return 0;
    }

    // 视频 / 相机模式
    cv::VideoCapture cap;
    myCamera camera(0);
    bool useCamera = source.empty();
    if (!useCamera)
        cap.open(source);

    // 视频模式带 save 参数：不弹窗，逐帧打印状态，结束保存最后一帧
    bool saveMode = !useCamera && argc > 3 && std::string(argv[3]) == "save";

    cv::Mat frame, lastFrame;
    int frameIdx = 0;
    while (true)
    {
        bool ok = useCamera ? camera.read(frame) : cap.read(frame);
        if (!ok)
            break;

        double timestampSeconds = useCamera
                                      ? -1.0
                                      : cap.get(cv::CAP_PROP_POS_MSEC) * 0.001;
        armor.detect(frame, timestampSeconds);

        if (saveMode)
        {
            cv::Point3f v = armor.velocity();
            std::cout << "frame " << frameIdx++
                      << ": state=" << armor.trackState()
                      << " num=" << armor.number()
                      << " dist=" << armor.distance() << "m"
                      << " speed="
                      << std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
                      << "m/s" << std::endl;
            frame.copyTo(lastFrame);
            continue;
        }

        cv::imshow("armor", frame);
        if (cv::waitKey(useCamera ? 1 : 30) == 27) // ESC 退出
            break;
    }
    if (saveMode && !lastFrame.empty())
    {
        cv::imwrite("result.jpg", lastFrame);
        std::cout << "saved last frame to result.jpg" << std::endl;
    }
    return 0;
}
