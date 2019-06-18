# CMakeLists.txt 用于生成 libbreakpad.so 库
cmake_minimum_required(VERSION 3.4.1)

set(BREAKPAD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# 添加使用的头文件
include_directories(
        ${BREAKPAD_ROOT}/src
        ${BREAKPAD_ROOT}/src/common/android/include
)

# 需要编译的文件列表
file(
        GLOB
        BREAKPAD_SOURCES_LIST
        ${BREAKPAD_ROOT}/src/client/linux/crash_generation/crash_generation_client.cc
        ${BREAKPAD_ROOT}/src/client/linux/dump_writer_common/thread_info.cc
        ${BREAKPAD_ROOT}/src/client/linux/dump_writer_common/ucontext_reader.cc
        ${BREAKPAD_ROOT}/src/client/linux/handler/exception_handler.cc
        ${BREAKPAD_ROOT}/src/client/linux/handler/minidump_descriptor.cc
        ${BREAKPAD_ROOT}/src/client/linux/log/log.cc
        ${BREAKPAD_ROOT}/src/client/linux/microdump_writer/microdump_writer.cc
        ${BREAKPAD_ROOT}/src/client/linux/minidump_writer/linux_dumper.cc
        ${BREAKPAD_ROOT}/src/client/linux/minidump_writer/linux_ptrace_dumper.cc
        ${BREAKPAD_ROOT}/src/client/linux/minidump_writer/minidump_writer.cc
        ${BREAKPAD_ROOT}/src/client/minidump_file_writer.cc
        ${BREAKPAD_ROOT}/src/common/convert_UTF.c
        ${BREAKPAD_ROOT}/src/common/md5.cc
        ${BREAKPAD_ROOT}/src/common/string_conversion.cc
        ${BREAKPAD_ROOT}/src/common/linux/elfutils.cc
        ${BREAKPAD_ROOT}/src/common/linux/file_id.cc
        ${BREAKPAD_ROOT}/src/common/linux/guid_creator.cc
        ${BREAKPAD_ROOT}/src/common/linux/linux_libc_support.cc
        ${BREAKPAD_ROOT}/src/common/linux/memory_mapped_file.cc
        ${BREAKPAD_ROOT}/src/common/linux/safe_readlink.cc
)

# 汇编文件列表
file(
        GLOB
        BREAKPAD_ASM_SOURCE_LIST
        ${BREAKPAD_ROOT}/src/common/android/breakpad_getcontext.S
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
