cmake_minimum_required(VERSION 3.16)
project(Task3YoloArmor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32 AND EXISTS "C:/msys64/mingw64/lib/cmake/opencv4")
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()

find_package(OpenCV REQUIRED)
add_subdirectory(io)
add_subdirectory(task3)

if(WIN32 AND MINGW)
    add_custom_target(bundle ALL
        COMMAND ${CMAKE_COMMAND} -DBIN_DIR=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                -P ${CMAKE_SOURCE_DIR}/cmake/copy_dlls.cmake
        DEPENDS example yolo_demo)
endif()
