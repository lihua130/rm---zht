# 题目2：OpenCV 传统视觉装甲板识别，封装为 myArmor 类
add_library(myarmor STATIC myArmor.cpp)
target_include_directories(myarmor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(myarmor PUBLIC ${OpenCV_LIBS})

add_executable(armor_demo main.cpp)
target_link_libraries(armor_demo PRIVATE myarmor mycamera)
if(COMMAND bundle_dlls)
    bundle_dlls(armor_demo)
endif()
