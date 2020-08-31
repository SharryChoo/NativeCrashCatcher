//
// Created by SharryChoo on 2020/8/31.
//

#ifndef NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H
#define NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H


struct CrashContext {
    siginfo_t siginfo;
    pid_t tid;  // the crashing thread.
    ucontext_t context;
#if !defined(__ARM_EABI__) && !defined(__mips__)
    // #ifdef this out because FP state is not part of user ABI for Linux ARM.
    // In case of MIPS Linux FP state is already part of ucontext_t so
    // 'float_state' is not required.
    fpstate_t float_state;
#endif
};

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
