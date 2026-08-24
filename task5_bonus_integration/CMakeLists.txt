cmake_minimum_required(VERSION 3.16)
project(RoboMasterVision CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MinGW(MSYS2) 环境下定位 pacman 安装的 OpenCV
if(WIN32 AND EXISTS "C:/msys64/mingw64/lib/cmake/opencv4")
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()

find_package(OpenCV REQUIRED)

# 可执行文件统一输出到 build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 题目5：父子级编译管理，各题目独立子目录
add_subdirectory(io)     # 题目1：myCamera 类
add_subdirectory(task2)  # 题目2：传统视觉装甲板识别 myArmor 类
add_subdirectory(task3)  # 题目3：YOLO 装甲板识别 myYolo 类
add_subdirectory(task4)  # 题目4：上下位机通信 myComm 类（电脑端）

# MinGW：所有 exe 链接完成后，统一把运行时 DLL 拷到 bin，免配 PATH 直接运行
if(WIN32 AND MINGW)
    add_custom_target(bundle ALL
        COMMAND ${CMAKE_COMMAND}
            -DBIN_DIR=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            -P ${CMAKE_SOURCE_DIR}/cmake/copy_dlls.cmake
        DEPENDS example armor_demo yolo_demo comm_demo
        COMMENT "bundle runtime dlls")
endif()
