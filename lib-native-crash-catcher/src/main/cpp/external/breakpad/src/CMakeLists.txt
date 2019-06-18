# CMakeLists.txt 用于生成 libbreakpad.so 库
cmake_minimum_required(VERSION 3.4.1)

# 添加使用的头文件
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/android/include
)

# 需要编译的文件列表
file(
        GLOB
        BREAKPAD_SOURCES_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/crash_generation/crash_generation_client.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/dump_writer_common/thread_info.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/dump_writer_common/ucontext_reader.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/handler/exception_handler.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/handler/minidump_descriptor.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/log/log.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/microdump_writer/microdump_writer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/minidump_writer/linux_dumper.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/minidump_writer/linux_ptrace_dumper.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/linux/minidump_writer/minidump_writer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/client/minidump_file_writer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/convert_UTF.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/md5.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/string_conversion.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/linux/elfutils.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/linux/file_id.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/linux/guid_creator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/linux/linux_libc_support.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/linux/memory_mapped_file.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/common/linux/safe_readlink.cc
)

# 汇编文件列表
file(
        GLOB
        BREAKPAD_ASM_SOURCE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/common/android/breakpad_getcontext.S
)

set_source_files_properties(${BREAKPAD_ASM_SOURCE_LIST} PROPERTIES LANGUAGE C)

# 生成 libbreakpad.so 库
add_library(
        breakpad
        SHARED
        ${BREAKPAD_SOURCES_LIST}
        ${BREAKPAD_ASM_SOURCE_LIST}
)

# 为 breakpad 添加链接库
target_link_libraries(
        breakpad
        log
)
