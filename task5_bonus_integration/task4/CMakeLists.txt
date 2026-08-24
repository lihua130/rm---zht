# 题目4：上下位机通信，封装为 myComm 类（电脑端）
# 单片机端参考代码位于 stm32/ 目录（不参与电脑端编译，烧录到 32 套件使用）
add_library(mycomm STATIC myComm.cpp)
target_include_directories(mycomm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(comm_demo main.cpp)
target_link_libraries(comm_demo PRIVATE mycomm myarmor mycamera ${OpenCV_LIBS})
if(COMMAND bundle_dlls)
    bundle_dlls(comm_demo)
endif()
