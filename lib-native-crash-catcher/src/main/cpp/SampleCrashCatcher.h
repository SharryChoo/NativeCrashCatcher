//
// Created by SharryChoo on 2020/8/31.
//

#ifndef NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H
#define NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H


class SampleCrashCatcher {

public:
    SampleCrashCatcher();
    ~SampleCrashCatcher();

private:
    bool hasInit = false;
    // 替换系统的信号处理栈
    void installAlterStack();
    // 恢复系统信号处理栈
    void restoreAlterStack();

    // 替换系统信号处理函数
    void installSysSigHandler();
    // 恢复系统信号处理函数
    void restoreSysSinHandler();
};


#endif //NATIVECRASHCATCHER_SAMPLECRASHCATCHER_H
