//
// Created by SharryChoo on 2020/8/31.
//

#ifndef NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H
#define NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H

#define MAX_BACKTRACE_DEPTH 64

typedef struct {
    // 栈帧数量
    int frame_num;
    // 是否找到目标栈
    int found_stack;
    // 上一层指针信息
    uintptr_t prev_pc;
    // 上一层指针信息
    uintptr_t prev_sp;
    // Crash 时的栈帧中 PC 寄存器的历史值(即当前代码段正在执行的函数地址)
    uintptr_t crash_sf_pc;
} BackTrace;

class SampleCrashCatcher {

public:
    SampleCrashCatcher();

    ~SampleCrashCatcher();

    // 重置成为内核默认的信号处理器
    static void restoreKernelDefaultSigHandler(int sig);

    // 恢复系统信号处理函数
    static void restoreSysSinHandler();

    // 替换系统的信号处理栈
    static void installAlterStack();

    // 恢复系统信号处理栈
    static void restoreAlterStack();

    // 替换系统信号处理函数
    static void installSysSigHandler();

};


#endif //NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H
