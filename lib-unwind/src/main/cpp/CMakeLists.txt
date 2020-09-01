cmake_minimum_required(VERSION 3.4.1)

# 设置工程名
project(unwind-core)

# 添加 cmake 参数
set(ENABLE_INPROCESS ON)
set(ENABLE_OUTOFPROCESS ON)
set(ENABLE_LIBCORKSCREW ON)
set(ENABLE_LIBUNWIND ON)
set(ENABLE_LIBUNWINDSTACK ON)
set(ENABLE_CXXABI ON)
set(ENABLE_STACKSCAN ON)
# 使用 C++ 11 no exceptions 的 STD 版本
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-exceptions")
if (${ENABLE_INPROCESS})
    add_definitions(-DENABLE_INPROCESS)
endif ()
if (${ENABLE_OUTOFPROCESS})
    add_definitions(-DENABLE_OUTOFPROCESS)
endif ()

# 链接 CMakeLists.txt 子目录
add_subdirectory(external/unwind)
list(APPEND LINK_LIBRARIES unwind)

# 添加要使用的头文件
include_directories(external/unwind/src)

# 生成动态库
add_library(
        unwind-catcher
        SHARED
        # 异常捕获的代码
        native-bridge.cpp
        SampleCrashCatcher.cpp
)

# 添加外部链接库
target_link_libraries(
        unwind-catcher
        # other lib linked.
        ${LINK_LIBRARIES}
        log
)