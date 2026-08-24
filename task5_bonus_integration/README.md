# RoboMaster 视觉组题目 1/2/3/5

本仓库使用 C++17、OpenCV 和 CMake，实现跨平台相机读取、传统视觉装甲板识别、YOLO 装甲板识别，以及跟踪、测距和姿态估计等发挥功能。

## 环境要求

- CMake 3.16或更高版本
- 支持 C++17的编译器
- OpenCV 4，需包含 `calib3d`、`dnn`、`highgui` 和 `video` 模块
- 题目 3训练脚本可选依赖：Python 3、Ultralytics

Windows已使用 MSYS2 MinGW64和 OpenCV 4.13构建验证。Ubuntu可使用系统编译器和 `libopencv-dev`；相机后端会在 Windows使用 DirectShow，在 Linux使用 V4L2。

## 编译

```bash
cmake -S . -B build
cmake --build build --parallel
```

生成的程序位于 `build/bin`。

## 题目 1：myCamera

实现位于 `io/`。`myCamera`的公有成员仅包含构造函数、析构函数和 `read`，所有私有变量均以下划线结尾。

```bash
./build/bin/example
```

默认打开 0号摄像头，按 Esc退出。

## 题目 2：传统视觉装甲板识别

实现位于 `task2/`，主要流程为颜色分离、灯条提取、几何配对、透视校正、数字模板识别和多帧投票。Kalman滤波用于跨帧目标关联、短暂丢失预测和恢复。

```bash
./build/bin/armor_demo red
./build/bin/armor_demo blue path/to/video.mp4
./build/bin/armor_demo blue path/to/image.jpg save
```

可生成包含移动和短暂遮挡的离线测试视频：

```bash
python tools/make_test_video.py test_video.mp4
./build/bin/armor_demo blue test_video.mp4 save
```

## 题目 3：YOLO装甲板识别

实现位于 `task3/`。程序通过 OpenCV DNN加载随仓库交付的 `task3/armor.onnx`，完成装甲板检测、NMS、框选和跟随提示。

```bash
./build/bin/yolo_demo task3/armor.onnx path/to/video.mp4
./build/bin/yolo_demo task3/armor.onnx path/to/image.jpg save
```

训练脚本为 `task3/train_yolo.py`，数据转换脚本为 `datasets/prepare_dataset.py`。题目 3和题目 5的独立交付包包含处理后的 YOLO数据集，可直接复现训练；Git仓库默认忽略数据图片以控制仓库体积。仅运行检测时不需要训练数据。

## 题目 5：发挥部分

- 根 `CMakeLists.txt`对子目录进行父子级编译管理。
- 题目 2、3、4分别封装为 `myArmor`、`myYolo`、`myComm`。
- 传统视觉结果使用四点轮廓框选倾斜装甲板。
- 使用 `SOLVEPNP_IPPE`解算长方形平面装甲板的距离和姿态。
- 画面显示随姿态变化的装甲板坐标轴。
- 单独的橙色箭头表示运动方向，箭头长度随速度增加。

默认内参根据 70度水平视场角估算，畸变系数默认为 0。未标定摄像头时，距离和姿态只用于算法演示； `setCameraMatrix`传入标定内参，并补充真实畸变参数。

## 工程结构

```text
io/          题目1：myCamera
task2/       题目2及题目5视觉发挥功能：myArmor
task3/       题目3：myYolo、训练脚本和ONNX模型
task4/       myComm电脑端
cmake/       Windows运行时依赖复制脚本
```

