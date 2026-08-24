# 题目 3：YOLO装甲板识别

`task3/myYolo.*`通过 OpenCV DNN加载 `task3/armor.onnx`，完成装甲板框选和跟随。`io/`为题目 1相机依赖。

```bash
cmake -S . -B build
cmake --build build --parallel
./build/bin/yolo_demo task3/armor.onnx path/to/video.mp4
./build/bin/yolo_demo task3/armor.onnx path/to/image.jpg save
```
