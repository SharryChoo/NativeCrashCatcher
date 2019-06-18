cmake_minimum_required(VERSION 3.4.1)

# 设置工程名
project(breakpad-core)

# 添加 cmake 参数
set(ENABLE_INPROCESS ON)
set(ENABLE_OUTOFPROCESS ON)
set(ENABLE_LIBCORKSCREW ON)
set(ENABLE_LIBUNWIND ON)
set(ENABLE_LIBUNWINDSTACK ON)
set(ENABLE_CXXABI ON)
set(ENABLE_STACKSCAN ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
if (${ENABLE_INPROCESS})
    add_definitions(-DENABLE_INPROCESS)
endif ()
if (${ENABLE_OUTOFPROCESS})
    add_definitions(-DENABLE_OUTOFPROCESS)
endif ()

# 链接 CMakeLists.txt 子目录
add_subdirectory(external/breakpad/src)
list(APPEND LINK_LIBRARIES breakpad)

# 添加要使用的头文件
include_directories(
        external/breakpad/src
        external/breakpad/src/common/android/include
)

# 生成动态库
add_library(
        native-crash-catcher
        SHARED
        native-crash-catcher.cpp
)

# 添加外部链接库
target_link_libraries(
        native-crash-catcher
        # other lib linked.
        ${LINK_LIBRARIES}
        log
)