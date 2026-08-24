# 由 bundle 目标调用：解析 bin 目录下所有 exe 的运行时依赖，
# 将 MinGW/MSYS2 的 DLL 拷到 bin 目录
# 用法：cmake -DBIN_DIR=<exe所在目录> -P copy_dlls.cmake
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

file(GLOB EXES "${BIN_DIR}/*.exe")

# 先清理上次拷入的旧 DLL，避免解析时同名路径冲突
file(GLOB OLD_DLLS "${BIN_DIR}/*.dll")
if(OLD_DLLS)
    file(REMOVE ${OLD_DLLS})
endif()

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${EXES}
    RESOLVED_DEPENDENCIES_VAR DEPS
    UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED
    DIRECTORIES "C:/msys64/mingw64/bin")

foreach(f IN LISTS DEPS)
    if(f MATCHES "[Mm][Ss][Yy][Ss]64")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${f}" "${BIN_DIR}/")
    endif()
endforeach()

if(UNRESOLVED)
    message(STATUS "unresolved dependencies: ${UNRESOLVED}")
endif()
