#include "myArmor.h"
#include <algorithm>
#include <cmath>

myArmor::myArmor(int enemyColor)
    : enemyColor_(enemyColor), lastCenter_(0, 0), hasTarget_(false),
      distance_(-1.0f), yawDeg_(0.0f), velocity_(0, 0, 0),
      prevPos_(0, 0, 0), prevTime_(0.0), hasPrevPos_(false),
      kf_(4, 2, 0, CV_32F), state_(NO_TARGET), hitCount_(0), lostCount_(0),
      stableNum_(0)
{
    buildTemplates();
    distCoeffs_ = cv::Mat::zeros(5, 1, CV_64F);

    // Kalman：状态 (x, y, vx, vy)，观测 (x, y)
    kf_.transitionMatrix = (cv::Mat_<float>(4, 4) <<
        1, 0, 1, 0,
        0, 1, 0, 1,
        0, 0, 1, 0,
        0, 0, 0, 1);
    kf_.measurementMatrix = (cv::Mat_<float>(2, 4) <<
        1, 0, 0, 0,
        0, 1, 0, 0);
    cv::setIdentity(kf_.processNoiseCov, cv::Scalar::all(1e-2));
    cv::setIdentity(kf_.measurementNoiseCov, cv::Scalar::all(1e-1));
    cv::setIdentity(kf_.errorCovPost, cv::Scalar::all(1));
}

myArmor::~myArmor() = default;

cv::Point2f myArmor::target() const
{
    return hasTarget_ ? lastCenter_ : cv::Point2f(-1, -1);
}

float myArmor::distance() const
{
    return hasTarget_ ? distance_ : -1.0f;
}

cv::Point3f myArmor::velocity() const
{
    return hasTarget_ ? velocity_ : cv::Point3f(0, 0, 0);
}

float myArmor::yaw() const
{
    return hasTarget_ ? yawDeg_ : 0.0f;
}

int myArmor::number() const
{
    return stableNum_;
}

int myArmor::trackState() const
{
    return (int)state_;
}

void myArmor::setCameraMatrix(double fx, double fy, double cx, double cy)
{
    cameraMatrix_ = (cv::Mat_<double>(3, 3) <<
        fx, 0, cx,
        0, fy, cy,
        0, 0, 1);
}

// 颜色分割：红色取 R-B，蓝色取 B-R，突出自发光灯条
cv::Mat myArmor::preprocess(const cv::Mat &frame)
{
    std::vector<cv::Mat> channels;
    cv::split(frame, channels);

    cv::Mat colorDiff;
    if (enemyColor_ == RED)
        cv::subtract(channels[2], channels[0], colorDiff); // R - B
    else
        cv::subtract(channels[0], channels[2], colorDiff); // B - R

    cv::Mat binary;
    cv::threshold(colorDiff, binary, 80, 255, cv::THRESH_BINARY);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    cv::dilate(binary, binary, kernel, cv::Point(-1, -1), 1);
    return binary;
}

// 从二值图中筛选灯条（细长的竖直旋转矩形）
std::vector<myArmor::LightBar> myArmor::findLightBars(const cv::Mat &binary)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<LightBar> bars;
    for (const auto &c : contours)
    {
        if (cv::contourArea(c) < 30.0)
            continue;

        cv::RotatedRect r = cv::minAreaRect(c);
        float length = std::max(r.size.width, r.size.height);
        float width = std::min(r.size.width, r.size.height);
        if (width < 2.0f)
            continue;

        float ratio = length / width;
        if (ratio < 2.0f || ratio > 15.0f) // 灯条细长
            continue;

        // 用顶点计算长边方向，避免 minAreaRect 角度歧义
        cv::Point2f pts[4];
        r.points(pts);
        cv::Point2f e1 = pts[1] - pts[0];
        cv::Point2f e2 = pts[2] - pts[1];
        cv::Point2f longEdge = (cv::norm(e1) >= cv::norm(e2)) ? e1 : e2;
        float angleX = std::atan2(longEdge.y, longEdge.x) * 180.0f / CV_PI; // (-180,180]
        // 线段无方向，归一化到 (-90,90]
        if (angleX > 90.0f)
            angleX -= 180.0f;
        else if (angleX <= -90.0f)
            angleX += 180.0f;
        // 与竖直方向夹角（带符号，>0 右倾，<0 左倾）
        float tilt = (angleX >= 0) ? (90.0f - angleX) : (-90.0f - angleX);
        if (std::fabs(tilt) > 35.0f) // 灯条应接近竖直
            continue;

        // 计算上下端点
        cv::Point2f unit = longEdge / (float)cv::norm(longEdge);
        cv::Point2f p1 = r.center + unit * (length * 0.5f);
        cv::Point2f p2 = r.center - unit * (length * 0.5f);

        LightBar bar;
        bar.center_ = r.center;
        bar.length_ = length;
        bar.tilt_ = tilt;
        bar.top_ = (p1.y < p2.y) ? p1 : p2;
        bar.bottom_ = (p1.y < p2.y) ? p2 : p1;
        bars.push_back(bar);
    }
    return bars;
}

// 灯条配对：满足平行、同高、间距比例合理的一对灯条构成装甲板
bool myArmor::matchPair(const LightBar &a, const LightBar &b,
                        std::vector<cv::Point2f> &corners)
{
    float avgLen = (a.length_ + b.length_) * 0.5f;

    if (std::fabs(a.tilt_ - b.tilt_) > 10.0f) // 近似平行
        return false;

    float lenRatio = std::min(a.length_, b.length_) / std::max(a.length_, b.length_);
    if (lenRatio < 0.5f) // 长度接近
        return false;

    if (std::fabs(a.center_.y - b.center_.y) > 0.5f * avgLen) // 基本同高
        return false;

    float dx = std::fabs(a.center_.x - b.center_.x);
    float distRatio = dx / avgLen;
    if (distRatio < 0.8f || distRatio > 4.5f) // 间距与灯条长度比例合理
        return false;

    const LightBar &left = (a.center_.x < b.center_.x) ? a : b;
    const LightBar &right = (a.center_.x < b.center_.x) ? b : a;
    corners = {left.top_, right.top_, right.bottom_, left.bottom_};
    return true;
}

// 数字识别：透视变换扶正装甲板区域，取中间数字与模板逐一匹配
int myArmor::recognizeNumber(const cv::Mat &frame,
                             const std::vector<cv::Point2f> &corners)
{
    if (templates_.empty())
        return 0;

    const int warpW = 120, warpH = 60;
    std::vector<cv::Point2f> dst = {{0, 0}, {(float)warpW, 0}, {(float)warpW, (float)warpH}, {0, (float)warpH}};
    cv::Mat M = cv::getPerspectiveTransform(corners, dst);

    cv::Mat warp, gray, binary;
    cv::warpPerspective(frame, warp, M, {warpW, warpH});
    cv::cvtColor(warp, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    // 中间 50% 宽度为数字区域
    cv::Rect roi(warpW / 4, warpH / 6, warpW / 2, warpH * 2 / 3);
    // 等比缩放到高 60 保持横纵比（不假设数字在几何中心，由后续笔迹归一化对齐）
    cv::Mat digit;
    {
        cv::Mat roiImg = binary(roi);
        float s = 60.0f / roiImg.rows;
        cv::resize(roiImg, digit, {(int)std::round(roiImg.cols * s), 60});
    }

    // 数字归一化：裁出笔迹外接框，等比缩放到高 48 后居中放入 40x60
    // 消除目标远近导致的尺度差异，与模板严格对齐
    {
        std::vector<cv::Point> pts;
        cv::findNonZero(digit, pts);
        if (pts.empty())
            return 0; // 无笔迹，放弃识别
        cv::Rect box = cv::boundingRect(pts);
        cv::Mat cropped = digit(box);
        float scale = 48.0f / box.height;
        int newW = std::min((int)std::round(box.width * scale), 36);
        cv::resize(cropped, cropped, {newW, 48});
        digit = cv::Mat(60, 40, CV_8UC1, cv::Scalar(0));
        cropped.copyTo(digit(cv::Rect((40 - newW) / 2, 6, newW, 48)));
    }

    int bestNum = 0;
    double bestScore = 0.55; // 匹配得分阈值

    // 正反两版都试：数字可能是亮字暗底或暗字亮底
    cv::Mat digitInv;
    cv::bitwise_not(digit, digitInv);

    // 每个数字有 3 种尺度模板，逐一匹配取最大得分
    for (int n = 0; n < 8; ++n)
    {
        for (int s = 0; s < 3; ++s)
        {
            cv::Mat r1, r2;
            cv::matchTemplate(digit, templates_[n * 3 + s], r1, cv::TM_CCOEFF_NORMED);
            cv::matchTemplate(digitInv, templates_[n * 3 + s], r2, cv::TM_CCOEFF_NORMED);
            double score = std::max(r1.at<float>(0, 0), r2.at<float>(0, 0));
            if (score > bestScore)
            {
                bestScore = score;
                bestNum = n + 1;
            }
        }
    }
    return bestNum;
}

// 程序内生成 1~8 白字黑底数字模板（40x60，getTextSize 居中，多尺度）
// 模板同样做笔迹归一化（高 48 居中），与 recognizeNumber 的归一化规则严格一致
void myArmor::buildTemplates()
{
    templates_.clear();
    const float scales[] = {2.0f, 2.35f, 2.7f}; // 多尺度容差（笔画粗细/风格差异）
    for (int i = 1; i <= 8; ++i)
    {
        std::string str = std::to_string(i);
        for (float s : scales)
        {
            int baseline = 0;
            cv::Size ts = cv::getTextSize(str, cv::FONT_HERSHEY_SIMPLEX, s, 3, &baseline);
            cv::Mat t(60, 40, CV_8UC1, cv::Scalar(0));
            cv::putText(t, str, {(40 - ts.width) / 2, (60 + ts.height) / 2},
                        cv::FONT_HERSHEY_SIMPLEX, s, cv::Scalar(255), 3);
            // 笔迹归一化：与识别侧同一规则，保证尺度严格对齐
            std::vector<cv::Point> pts;
            cv::findNonZero(t, pts);
            if (pts.empty())
                continue;
            cv::Rect box = cv::boundingRect(pts);
            cv::Mat cropped = t(box);
            float sc = 48.0f / box.height;
            int nw = std::min((int)std::round(box.width * sc), 36);
            cv::resize(cropped, cropped, {nw, 48});
            t = cv::Mat(60, 40, CV_8UC1, cv::Scalar(0));
            cropped.copyTo(t(cv::Rect((40 - nw) / 2, 6, nw, 48)));
            templates_.push_back(t);
        }
    }
}

// 跟随提示：画面中心准星 + 目标偏移方向与像素数
void myArmor::drawFollow(cv::Mat &frame, const cv::Point2f &center)
{
    cv::Point2f imgCenter(frame.cols * 0.5f, frame.rows * 0.5f);
    cv::drawMarker(frame, imgCenter, cv::Scalar(255, 255, 255),
                   cv::MARKER_CROSS, 30, 2);

    float dx = center.x - imgCenter.x;
    float dy = center.y - imgCenter.y;

    cv::arrowedLine(frame, imgCenter, center, cv::Scalar(0, 255, 255), 2);

    std::string dir;
    if (dx > 30)
        dir += "RIGHT ";
    else if (dx < -30)
        dir += "LEFT ";
    if (dy > 30)
        dir += "DOWN";
    else if (dy < -30)
        dir += "UP";
    if (dir.empty())
        dir = "LOCKED";

    char info[64];
    std::snprintf(info, sizeof(info), "follow: %s (dx=%d, dy=%d)",
                  dir.c_str(), (int)dx, (int)dy);
    cv::putText(frame, info, {10, 30}, cv::FONT_HERSHEY_SIMPLEX,
                0.8, cv::Scalar(0, 255, 255), 2);
}

// PnP 解算：由装甲板四点求距离、姿态与运动速度
bool myArmor::solvePose(const std::vector<cv::Point2f> &corners,
                        const cv::Size &imgSize,
                        double timestampSeconds)
{
    if (corners.size() != 4)
        return false;

    // 未标定时按水平视场角 70° 估计内参
    if (cameraMatrix_.empty())
    {
        double f = imgSize.width / (2.0 * std::tan(70.0 * CV_PI / 360.0));
        cameraMatrix_ = (cv::Mat_<double>(3, 3) <<
            f, 0, imgSize.width / 2.0,
            0, f, imgSize.height / 2.0,
            0, 0, 1);
    }

    // 装甲板灯条中心物理间距（mm）：小装甲 135x55，大装甲 225x55，按像素宽高比粗判
    float wpx = ((float)cv::norm(corners[0] - corners[1]) +
                 (float)cv::norm(corners[2] - corners[3])) * 0.5f;
    float hpx = ((float)cv::norm(corners[0] - corners[3]) +
                 (float)cv::norm(corners[1] - corners[2])) * 0.5f;
    float realW = (hpx > 1.0f && wpx / hpx > 3.2f) ? 225.0f : 135.0f;
    const float realH = 55.0f;

    // 3D 模型点（装甲板坐标系：中心为原点，x 向右，y 向下，z 朝外）
    std::vector<cv::Point3f> obj = {
        {-realW / 2, -realH / 2, 0}, {realW / 2, -realH / 2, 0},
        {realW / 2, realH / 2, 0}, {-realW / 2, realH / 2, 0}};

    cv::Mat rvec, tvec;
    // 装甲板为长方形平面目标：SOLVEPNP_IPPE 适用于任意共面点集，
    // 比 ITERATIVE 更不易陷入局部极小（IPPE_SQUARE 仅适用正方形，不可用）
    if (!cv::solvePnP(obj, corners, cameraMatrix_, distCoeffs_,
                      rvec, tvec, false, cv::SOLVEPNP_IPPE))
        return false;

    rvec_ = rvec;
    tvec_ = tvec * 0.001; // mm -> m
    distance_ = (float)(cv::norm(tvec) * 0.001);

    // 偏航角：旋转矩阵中取绕竖直轴分量，归一化到 (-90,90]（0 为正对相机）
    cv::Mat R;
    cv::Rodrigues(rvec_, R);
    float yaw = (float)(std::atan2(R.at<double>(0, 2), R.at<double>(2, 2)) * 180.0 / CV_PI);
    if (yaw > 90.0f)
        yaw -= 180.0f;
    else if (yaw < -90.0f)
        yaw += 180.0f;
    yawDeg_ = yaw;

    // 运动速度：相邻帧位置差 / 时间差，指数平滑
    double now = timestampSeconds >= 0.0
                     ? timestampSeconds
                     : (double)cv::getTickCount() / cv::getTickFrequency();
    cv::Point3f pos((float)tvec_.at<double>(0),
                    (float)tvec_.at<double>(1),
                    (float)tvec_.at<double>(2));
    if (hasPrevPos_ && now > prevTime_)
    {
        cv::Point3f v = (pos - prevPos_) / (float)(now - prevTime_);
        velocity_ = 0.6f * velocity_ + 0.4f * v;
    }
    prevPos_ = pos;
    prevTime_ = now;
    hasPrevPos_ = true;
    return true;
}

// Kalman 多帧跟踪：目标关联、状态机（NEW/TRACKING/TEMP_LOST/NO_TARGET）
void myArmor::updateTrack(bool found, const cv::Point2f &center)
{
    if (found)
    {
        cv::Point2f predPt(kf_.statePre.at<float>(0), kf_.statePre.at<float>(1));
        // 与预测位置偏差过大 -> 视为新目标，重置滤波与投票
        if (state_ == TRACKING && cv::norm(center - predPt) > 150.0)
        {
            hitCount_ = 0;
            numHistory_.clear();
            stableNum_ = 0;
            hasPrevPos_ = false;
            velocity_ = cv::Point3f(0, 0, 0);
        }
        if (hitCount_ == 0)
        {
            kf_.statePost.at<float>(0) = center.x;
            kf_.statePost.at<float>(1) = center.y;
            kf_.statePost.at<float>(2) = 0;
            kf_.statePost.at<float>(3) = 0;
        }
        kf_.correct((cv::Mat_<float>(2, 1) << center.x, center.y));

        ++hitCount_;
        lostCount_ = 0;
        state_ = (hitCount_ >= 3) ? TRACKING : NEW_TARGET;
    }
    else
    {
        ++lostCount_;
        hitCount_ = 0;
        if (state_ == NO_TARGET || lostCount_ > 8) // 丢失超过 8 帧彻底放弃
        {
            state_ = NO_TARGET;
            numHistory_.clear();
            stableNum_ = 0;
            hasPrevPos_ = false;
            velocity_ = cv::Point3f(0, 0, 0);
        }
        else
        {
            state_ = TEMP_LOST; // 短暂丢失，靠预测位置维持
        }
    }
}

// 数字多帧投票：最近 5 帧内某数字出现 >=3 次才确认
int myArmor::voteNumber(int num)
{
    if (num > 0)
    {
        numHistory_.push_back(num);
        if (numHistory_.size() > 5)
            numHistory_.pop_front();
    }
    int cnt[9] = {0};
    for (int v : numHistory_)
        ++cnt[v];
    for (int v = 1; v <= 8; ++v)
        if (cnt[v] >= 3)
            stableNum_ = v;
    return stableNum_;
}

// 绘制：跟踪状态、数字（确认值/单帧值）、距离/姿态/速度、装甲板坐标系坐标轴
void myArmor::drawInfo(cv::Mat &frame, const std::vector<cv::Point2f> &corners,
                       int rawNum)
{
    const char *stateStr[] = {"no target", "new target", "tracking", "lost"};
    cv::Scalar stateColor = (state_ == TRACKING) ? cv::Scalar(0, 255, 0)
                                                 : cv::Scalar(0, 255, 255);
    cv::putText(frame, stateStr[state_], {10, 60}, cv::FONT_HERSHEY_SIMPLEX,
                0.7, stateColor, 2);

    // 数字：优先显示投票确认值；未确认时显示单帧识别值并加 ? 后缀
    std::string label;
    if (stableNum_ > 0)
        label = std::to_string(stableNum_);
    else if (rawNum > 0)
        label = std::to_string(rawNum) + "?";
    else
        label = "?";
    cv::Point2f labelPos(
        std::clamp(corners[0].x, 5.0f, (float)std::max(5, frame.cols - 45)),
        std::clamp(corners[0].y - 10.0f, 25.0f, (float)std::max(25, frame.rows - 5)));
    cv::putText(frame, label, labelPos,
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

    // 固定信息区，避免目标靠近边缘时文字被裁切或与装甲板重叠
    char poseInfo[80];
    std::snprintf(poseInfo, sizeof(poseInfo), "distance=%.2fm  yaw=%.1fdeg",
                  distance_, yawDeg_);
    cv::putText(frame, poseInfo, {10, 90}, cv::FONT_HERSHEY_SIMPLEX,
                0.58, cv::Scalar(255, 255, 0), 2);

    const float speed = (float)cv::norm(velocity_);
    char motionInfo[96];
    std::snprintf(motionInfo, sizeof(motionInfo),
                  "speed=%.2fm/s  v=(%.2f,%.2f,%.2f)",
                  speed, velocity_.x, velocity_.y, velocity_.z);
    cv::putText(frame, motionInfo, {10, 115}, cv::FONT_HERSHEY_SIMPLEX,
                0.52, cv::Scalar(0, 165, 255), 2);

    // 装甲板自身坐标系坐标轴（x 红向右 / y 绿向下 / z 蓝朝外）
    if (!rvec_.empty() && !tvec_.empty() && !cameraMatrix_.empty())
    {
        cv::drawFrameAxes(frame, cameraMatrix_, distCoeffs_, rvec_, tvec_, 0.05f, 2);

        // 运动箭头（题目5.4）：起点装甲板中心，方向=速度方向，长度与速度大小成比例
        // 实现：把“中心”与“中心 + 0.5s 位移”两个相机系 3D 点投影回图像连线
        if (speed > 0.05f) // 低于 5cm/s 视为静止，避免噪声抖动
        {
            cv::Point3d p0(tvec_.at<double>(0), tvec_.at<double>(1), tvec_.at<double>(2));
            cv::Point3d v(velocity_.x, velocity_.y, velocity_.z);
            std::vector<cv::Point3d> pts3d = {p0, p0 + v * 0.5};
            // 注意：OpenCV 4.13 内部以 CV_64FC2 创建输出，必须用 Point2d 接收
            std::vector<cv::Point2d> pts2d;
            cv::projectPoints(pts3d, cv::Mat::zeros(3, 1, CV_64F),
                              cv::Mat::zeros(3, 1, CV_64F),
                              cameraMatrix_, distCoeffs_, pts2d);
            cv::Point2d delta = pts2d[1] - pts2d[0];
            double projectedLength = cv::norm(delta);
            if (projectedLength > 1e-3)
            {
                // 箭头长度与速度成比例，并设置显示上下限；靠近边缘时裁切到画面内。
                double arrowLength = std::clamp((double)speed * 90.0, 18.0, 120.0);
                // 平移到装甲板下方绘制，避免与姿态坐标轴重合；靠近底边时改画在上方。
                double verticalOffset = pts2d[0].y + 45.0 < frame.rows - 5 ? 45.0 : -45.0;
                cv::Point2d arrowOrigin = pts2d[0] + cv::Point2d(0.0, verticalOffset);
                cv::Point start(cvRound(arrowOrigin.x), cvRound(arrowOrigin.y));
                cv::Point end(cvRound(arrowOrigin.x + delta.x / projectedLength * arrowLength),
                              cvRound(arrowOrigin.y + delta.y / projectedLength * arrowLength));
                cv::Rect bounds(2, 2, std::max(1, frame.cols - 4),
                                std::max(1, frame.rows - 4));
                if (cv::clipLine(bounds, start, end))
                    cv::arrowedLine(frame, start, end, cv::Scalar(0, 165, 255),
                                    3, cv::LINE_AA, 0, 0.18);
            }
        }
    }
}

bool myArmor::detect(cv::Mat &frame, double timestampSeconds)
{
    // Kalman 预测本帧目标位置（用于目标关联与丢失维持）
    cv::Mat prediction = kf_.predict();
    cv::Point2f predPt(prediction.at<float>(0), prediction.at<float>(1));

    cv::Mat binary = preprocess(frame);
    std::vector<LightBar> bars = findLightBars(binary);

    // 画出所有候选灯条（蓝色细框）
    for (const auto &bar : bars)
        cv::line(frame, bar.top_, bar.bottom_, cv::Scalar(255, 0, 0), 2);

    // 枚举灯条对：跟踪中优先选靠近预测位置的目标，否则选最大的一对
    bool found = false;
    float bestScore = 0.0f;
    std::vector<cv::Point2f> bestCorners;
    cv::Point2f bestCenter(0, 0);
    for (size_t i = 0; i < bars.size(); ++i)
    {
        for (size_t j = i + 1; j < bars.size(); ++j)
        {
            std::vector<cv::Point2f> corners;
            if (!matchPair(bars[i], bars[j], corners))
                continue;

            cv::Point2f center = (corners[0] + corners[1] +
                                  corners[2] + corners[3]) * 0.25f;
            float score = bars[i].length_ + bars[j].length_;
            if ((state_ == TRACKING || state_ == TEMP_LOST) &&
                cv::norm(center - predPt) < 150.0)
                score += 200.0f; // 靠近预测位置的目标加权，保证跟踪连续

            if (score > bestScore)
            {
                bestScore = score;
                bestCorners = corners;
                bestCenter = center;
                found = true;
            }
        }
    }

    updateTrack(found, bestCenter);

    if (found)
    {
        // 绿色框选整块装甲板
        for (int i = 0; i < 4; ++i)
            cv::line(frame, bestCorners[i], bestCorners[(i + 1) % 4],
                     cv::Scalar(0, 255, 0), 2);

        int rawNum = recognizeNumber(frame, bestCorners); // 单帧数字
        voteNumber(rawNum);                               // 多帧投票确认
        solvePose(bestCorners, frame.size(), timestampSeconds); // 测距 / 姿态 / 速度
        drawInfo(frame, bestCorners, rawNum);             // 状态与数值信息
        drawFollow(frame, bestCenter);                   // 跟随提示

        lastCenter_ = bestCenter;
        hasTarget_ = true;
    }
    else if (state_ == TEMP_LOST)
    {
        // 短暂丢失：画出 Kalman 预测位置
        cv::drawMarker(frame, predPt, cv::Scalar(0, 255, 255),
                       cv::MARKER_TILTED_CROSS, 30, 2);
        cv::putText(frame, "lost, predicting...", {10, 60},
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
        hasTarget_ = false;
        distance_ = -1.0f;
    }
    else
    {
        hasTarget_ = false;
        distance_ = -1.0f;
        cv::putText(frame, "no target", {10, 30}, cv::FONT_HERSHEY_SIMPLEX,
                    0.8, cv::Scalar(0, 0, 255), 2);
    }
    return found;
}
