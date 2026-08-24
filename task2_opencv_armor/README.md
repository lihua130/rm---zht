# 题目 2：传统视觉装甲板识别

`task2/myArmor.*`实现灯条提取、装甲板配对、倾斜框选、数字识别、多帧投票及 Kalman跟踪。`io/`为题目 1相机依赖。

```bash
cmake -S . -B build
cmake --build build --parallel
./build/bin/armor_demo red
./build/bin/armor_demo blue path/to/video.mp4
./build/bin/armor_demo blue path/to/image.jpg save
```


依赖 C++17、CMake 3.16+和 OpenCV 4。
