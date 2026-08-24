#ifndef MYARMOR_H
#define MYARMOR_H

#include <opencv2/opencv.hpp>
#include <deque>
#include <vector>

// myArmor：OpenCV 传统视觉装甲板识别类
// 功能：灯条检测 -> 配对 -> 框选装甲板 -> 数字识别(多尺度模板+多帧投票)
//       -> Kalman 多帧跟踪 -> PnP 测距/3D 姿态 -> 运动速度 -> 跟随提示
//
// 坐标系定义（相机坐标系）：x 向右，y 向下，z 朝正前方；
// tvec 为目标中心在该坐标系下的位置（单位：米），速度同系。
class myArmor
{
public:
    enum Color
    {
        RED = 0,
        BLUE = 1
    };

    // 跟踪状态
    enum TrackState
    {
        NO_TARGET = 0,  // 无目标
        NEW_TARGET = 1, // 新目标（命中未满 3 帧，未确认）
        TRACKING = 2,   // 稳定跟踪中
        TEMP_LOST = 3   // 短暂丢失（用预测位置维持）
    };

    // 构造函数：enemyColor 为敌方装甲颜色（myArmor::RED / myArmor::BLUE）
    explicit myArmor(int enemyColor = RED);
    ~myArmor();

    // 检测一帧图像：框选装甲板、识别数字、跟踪、测距、输出跟随提示（结果绘制在 frame 上）
    // 返回 true 表示本帧识别到装甲板
    bool detect(cv::Mat &frame, double timestampSeconds = -1.0);

    // 获取最近一次检测到的装甲板中心像素坐标（无目标时返回 (-1,-1)）
    // 供题目4 下传单片机使用
    cv::Point2f target() const;

    float distance() const;        // 目标距离（米），无目标返回 -1
    cv::Point3f velocity() const;  // 目标运动速度（相机坐标系，米/秒）
    float yaw() const;             // 目标偏航角（度，0 为正对相机）
    int number() const;            // 多帧投票确认的数字（1~8），未确认返回 0
    int trackState() const;        // 跟踪状态（TrackState）

    // 相机标定后设置内参；不设置则按水平视场角 70° 自动估计
    void setCameraMatrix(double fx, double fy, double cx, double cy);

private:
    struct LightBar // 灯条结构体
    {
        cv::Point2f center_; // 中心
        cv::Point2f top_;    // 上端点
        cv::Point2f bottom_; // 下端点
        float length_;       // 长度
        float tilt_;         // 与竖直方向夹角（度，0 为竖直）
    };

    cv::Mat preprocess(const cv::Mat &frame);                   // 颜色分割 + 形态学
    std::vector<LightBar> findLightBars(const cv::Mat &binary); // 轮廓筛选灯条
    bool matchPair(const LightBar &a, const LightBar &b,
                   std::vector<cv::Point2f> &corners);           // 灯条配对成装甲板
    int recognizeNumber(const cv::Mat &frame,
                        const std::vector<cv::Point2f> &corners); // 透视变换 + 模板匹配识别数字
    void buildTemplates();                                        // 程序内生成 1~8 数字模板（多尺度）
    void drawFollow(cv::Mat &frame, const cv::Point2f &center);   // 跟随提示（偏移方向/像素）

    bool solvePose(const std::vector<cv::Point2f> &corners,
                   const cv::Size &imgSize,
                   double timestampSeconds);         // PnP 解算距离与姿态
    void updateTrack(bool found, const cv::Point2f &center); // Kalman 多帧跟踪
    int voteNumber(int num);                          // 数字多帧投票
    void drawInfo(cv::Mat &frame, const std::vector<cv::Point2f> &corners,
                  int rawNum);                          // 绘制数字/距离/姿态/速度/坐标轴

    int enemyColor_;                 // 敌方颜色
    std::vector<cv::Mat> templates_; // 数字模板（1~8，多尺度）
    cv::Point2f lastCenter_;         // 最近一次装甲中心
    bool hasTarget_;                 // 最近一次是否存在目标

    // ---- PnP / 测距 / 姿态 ----
    cv::Mat cameraMatrix_; // 相机内参（detect 中按图像尺寸懒初始化）
    cv::Mat distCoeffs_;   // 畸变系数（默认全 0）
    cv::Mat rvec_;         // 旋转向量
    cv::Mat tvec_;         // 平移向量（米）
    float distance_;       // 距离（米）
    float yawDeg_;         // 偏航角（度）

    // ---- 运动速度 ----
    cv::Point3f velocity_; // 速度（米/秒）
    cv::Point3f prevPos_;  // 上一帧位置（米）
    double prevTime_;      // 上一帧时间戳（秒）
    bool hasPrevPos_;      // 是否有上一帧位置

    // ---- 多帧跟踪（Kalman：状态 x,y,vx,vy，观测 x,y）----
    cv::KalmanFilter kf_;
    TrackState state_;
    int hitCount_;  // 连续命中帧数
    int lostCount_; // 连续丢失帧数

    // ---- 数字多帧投票 ----
    std::deque<int> numHistory_; // 最近若干帧识别结果
    int stableNum_;              // 投票确认的数字
};

#endif // MYARMOR_H
