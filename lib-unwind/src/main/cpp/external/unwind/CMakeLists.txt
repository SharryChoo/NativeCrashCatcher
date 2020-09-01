# CMakeLists.txt 用于生成 libbreakpad.so 库
cmake_minimum_required(VERSION 3.4.1)

# 添加使用的头文件
include_directories(
        src
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 需要编译的文件列表
file(
        GLOB
        UNWIND_SOURCES_LIST
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# 生成 unwind.a 的库
add_library(
        unwind
        STATIC
        ${UNWIND_SOURCES_LIST}
)

# 为 breakpad 添加链接库
target_link_libraries(
        unwind
        log
)