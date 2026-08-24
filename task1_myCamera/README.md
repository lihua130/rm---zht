# 题目 1：myCamera

本包实现 Windows/Ubuntu自适应的 `myCamera`类。公有成员仅包含构造函数、析构函数和 `read`，示例位于 `io/example.cpp`。

```bash
cmake -S . -B build
cmake --build build --parallel
./build/bin/example
```

默认打开 0号摄像头，按 Esc退出。依赖 C++17、CMake 3.16+和 OpenCV 4。
