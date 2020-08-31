//
// Created by SharryChoo on 2020/8/31.
//

#include <asm/signal.h>
#include "SampleCrashCatcher.h"


SampleCrashCatcher::SampleCrashCatcher() {
    // 1. 替换系统的信号处理栈
    installAlterStack();
    // 2. 替换系统信号处理函数
    installSysSigHandler();
}

SampleCrashCatcher::~SampleCrashCatcher() {
    restoreAlterStack();
    restoreSysSinHandler();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// 替换信号处理栈
////////////////////////////////////////////////////////////////////////////////////////////////

// InstallAlternateStackLocked will store the newly installed stack in new_stack
// and (if it exists) the previously installed stack in old_stack.
stack_t old_stack;
stack_t new_stack;
bool stack_installed = false;

void SampleCrashCatcher::installAlterStack() {

}

void SampleCrashCatcher::restoreAlterStack() {

}


////////////////////////////////////////////////////////////////////////////////////////////////
// 替换信号处理函数
////////////////////////////////////////////////////////////////////////////////////////////////

// The list of signals which we consider to be crashes. The default action for
// all these signals must be Core (see man 7 signal) because we rethrow the
// signal after handling it and expect that it'll be fatal.
const int kExceptionSignals[] = {
        SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTRAP
};
const int kNumHandledSignals =
        sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
struct sigaction old_handlers[kNumHandledSignals];

void SampleCrashCatcher::installSysSigHandler() {

}

void SampleCrashCatcher::restoreSysSinHandler() {

}
