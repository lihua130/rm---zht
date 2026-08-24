# 题目1：myCamera 类库 + 示例程序
add_library(mycamera STATIC myCamera.cpp)
target_include_directories(mycamera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(mycamera PUBLIC ${OpenCV_LIBS})

add_executable(example example.cpp)
target_link_libraries(example PRIVATE mycamera)
if(COMMAND bundle_dlls)
    bundle_dlls(example)
endif()
