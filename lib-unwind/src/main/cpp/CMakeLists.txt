cmake_minimum_required(VERSION 3.4.1)

# 设置工程名
project(unwind-core)

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