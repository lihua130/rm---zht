# 题目3：YOLO 装甲板识别，封装为 myYolo 类（ONNX + OpenCV DNN 推理）
add_library(myyolo STATIC myYolo.cpp)
target_include_directories(myyolo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(myyolo PUBLIC ${OpenCV_LIBS})

add_executable(yolo_demo main.cpp)
target_link_libraries(yolo_demo PRIVATE myyolo mycamera)
if(COMMAND bundle_dlls)
    bundle_dlls(yolo_demo)
endif()
