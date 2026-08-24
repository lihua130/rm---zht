#include "myYolo.h"
#include <iostream>

myYolo::myYolo(const std::string &onnxPath, float confThreshold)
    : confThreshold_(confThreshold), nmsThreshold_(0.45f), inputSize_(640), loaded_(false)
{
    try
    {
        net_ = cv::dnn::readNetFromONNX(onnxPath);
        loaded_ = !net_.empty();
    }
    catch (const cv::Exception &e)
    {
        std::cerr << "[myYolo] load model failed: " << e.what() << std::endl;
    }
    if (!loaded_)
        std::cerr << "[myYolo] cannot load model: " << onnxPath << std::endl;
}

myYolo::~myYolo() = default;

bool myYolo::detect(cv::Mat &frame)
{
    if (!loaded_)
    {
        cv::putText(frame, "model not loaded", {10, 30},
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
        return false;
    }

    // 前处理：resize 到 640x640，归一化，BGR
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0 / 255.0,
                                          {inputSize_, inputSize_},
                                          cv::Scalar(), true, false);
    net_.setInput(blob);
    cv::Mat out = net_.forward(); // YOLOv8: 1 x (4+nc) x 8400

    // 转置为 8400 x (4+nc)
    cv::Mat det(out.size[1], out.size[2], CV_32F, out.ptr<float>());
    cv::transpose(det, det);

    float scaleX = frame.cols / (float)inputSize_;
    float scaleY = frame.rows / (float)inputSize_;

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    for (int i = 0; i < det.rows; ++i)
    {
        const float *row = det.ptr<float>(i);
        // 单类别：第 5 列为类别得分；多类别取最大类别得分
        float score = 0.0f;
        for (int c = 4; c < det.cols; ++c)
            score = std::max(score, row[c]);
        if (score < confThreshold_)
            continue;

        float cx = row[0] * scaleX, cy = row[1] * scaleY;
        float w = row[2] * scaleX, h = row[3] * scaleY;
        boxes.emplace_back((int)(cx - w / 2), (int)(cy - h / 2), (int)w, (int)h);
        scores.push_back(score);
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, confThreshold_, nmsThreshold_, indices);

    bool found = !indices.empty();
    if (found)
    {
        // 取得分最高的框作为主要跟随目标
        int best = indices[0];
        for (int idx : indices)
        {
            cv::rectangle(frame, boxes[idx], cv::Scalar(0, 255, 0), 2);
            char text[32];
            std::snprintf(text, sizeof(text), "armor %.2f", scores[idx]);
            cv::putText(frame, text, {boxes[idx].x, boxes[idx].y - 5},
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            if (scores[idx] > scores[best])
                best = idx;
        }

        // 跟随：画面中心 -> 目标中心
        cv::Point2f imgCenter(frame.cols * 0.5f, frame.rows * 0.5f);
        cv::Point2f target(boxes[best].x + boxes[best].width * 0.5f,
                           boxes[best].y + boxes[best].height * 0.5f);
        cv::drawMarker(frame, imgCenter, cv::Scalar(255, 255, 255),
                       cv::MARKER_CROSS, 30, 2);
        cv::arrowedLine(frame, imgCenter, target, cv::Scalar(0, 255, 255), 2);

        float dx = target.x - imgCenter.x, dy = target.y - imgCenter.y;
        std::string dir;
        if (dx > 30) dir += "RIGHT ";
        else if (dx < -30) dir += "LEFT ";
        if (dy > 30) dir += "DOWN";
        else if (dy < -30) dir += "UP";
        if (dir.empty()) dir = "LOCKED";

        char info[64];
        std::snprintf(info, sizeof(info), "follow: %s (dx=%d, dy=%d)",
                      dir.c_str(), (int)dx, (int)dy);
        cv::putText(frame, info, {10, 30}, cv::FONT_HERSHEY_SIMPLEX,
                    0.8, cv::Scalar(0, 255, 255), 2);
    }
    else
    {
        cv::putText(frame, "no target", {10, 30}, cv::FONT_HERSHEY_SIMPLEX,
                    0.8, cv::Scalar(0, 0, 255), 2);
    }
    return found;
}
