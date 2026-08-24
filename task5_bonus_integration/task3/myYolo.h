#ifndef MYYOLO_H
#define MYYOLO_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

// myYolo：YOLOv8(ONNX) 装甲板识别类
// 功能：加载训练导出的 ONNX 模型，实现装甲板框选与跟随
class myYolo
{
public:
    // 构造函数：onnxPath 模型路径，confThreshold 置信度阈值
    explicit myYolo(const std::string &onnxPath, float confThreshold = 0.4f);
    ~myYolo();

    // 检测一帧：框选装甲板 + 跟随提示（绘制在 frame 上），返回 true 表示有目标
    bool detect(cv::Mat &frame);

private:
    cv::dnn::Net net_;    // DNN 推理网络
    float confThreshold_; // 置信度阈值
    float nmsThreshold_;  // NMS IOU 阈值
    int inputSize_;       // 模型输入边长（640）
    bool loaded_;         // 模型是否加载成功
};

#endif // MYYOLO_H
