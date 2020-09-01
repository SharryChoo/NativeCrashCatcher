#include <csignal>
#include <cstring>
#include <algorithm>
#include "SampleCrashCatcher.h"
#include "Constants.h"
#include "linux_syscall_support.h"

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
    // 已经安装了, 不做任何处理
    if (stack_installed) {
        return;
    }
    // 开辟空间, 用于存储之前的信号找
    memset(&old_stack, 0, sizeof(old_stack));
    // 开辟一个新的信号栈
    memset(&new_stack, 0, sizeof(new_stack));

    // SIGSTKSZ may be too small to prevent the signal handlers from overrunning
    // the alternative stack. Ensure that the size of the alternative stack is
    // large enough.
    // 描述信号栈的最小值
    static const unsigned minimumSigStackSize = std::max(16384, SIGSTKSZ);

    // 若当前不存在信号栈 || 或者信号栈太小, 则重新创建信号栈
    if (sys_sigaltstack(NULL, &old_stack) == -1 || !old_stack.ss_sp ||
        old_stack.ss_size < minimumSigStackSize) {
        // 分配一个连续的空间, 作为信号栈内存
        new_stack.ss_sp = calloc(1, minimumSigStackSize);
        new_stack.ss_size = minimumSigStackSize;
        // 将进程的信号处理堆栈替换成我们新创建的 new_stack
        if (sys_sigaltstack(&new_stack, NULL) == -1) {
            free(new_stack.ss_sp);
            return;
        }
        stack_installed = true;
//        ALOGI("===============installAlterStack success================");
    }
}

void SampleCrashCatcher::restoreAlterStack() {
    // 如果没有安装, 不做任何处理
    if (!stack_installed) {
        return;
    }
    stack_t current_stack;
    if (sys_sigaltstack(NULL, &current_stack) == -1) {
        return;
    }

    // Only restore the old_stack if the current alternative stack is the one
    // installed by the call to InstallAlternateStackLocked.
    if (current_stack.ss_sp == new_stack.ss_sp) {
        // 将之替换成之前的任务栈
        if (old_stack.ss_sp) {
            if (sys_sigaltstack(&old_stack, NULL) == -1) {
                return;
            }
        }
            // 之前信号栈不存在 ss_sp, 则创建一个不可用的注入
        else {
            stack_t disable_stack;
            disable_stack.ss_flags = SS_DISABLE;
            if (sys_sigaltstack(&disable_stack, NULL) == -1) {
                return;
            }
        }
    }
    // 释放新信号栈的 ss_sp
    free(new_stack.ss_sp);
    stack_installed = false;
//    ALOGI("===============restoreAlterStack success================");
}


////////////////////////////////////////////////////////////////////////////////////////////////
// 替换信号处理函数
////////////////////////////////////////////////////////////////////////////////////////////////

// The list of signals which we consider to be crashes. The default action for
// all these signals must be Core (see man 7 signal) because we rethrow the
// signal after handling it and expect that it'll be fatal.
const static int gExceptionSignals[] = {
        SIGILL,  // 4. 非法指令
        SIGTRAP, // 5. 断点
        SIGABRT, // 6, 进程遇到了预期错误，主动abort杀掉进程。
        SIGBUS,  // 7, 地址无效
        SIGFPE,  // 8. 浮点运算错误
        SIGSEGV, // 11. 空指针, 野指针等
};
const static int gNumHandledSignals = sizeof(gExceptionSignals) / sizeof(gExceptionSignals[0]);
struct sigaction old_handlers[gNumHandledSignals];
bool handlers_installed = false;
CrashContext gCrashContext;

bool dumpStack() {
    // 利用 unwind 库进行栈回溯
//    unwind_t uwind;
    return false;
}

// 1. 第一个参数为信号值
// 2. 第二个参数为信号的一些具体信息
// 3. 第三个参数为一些上下文信息
void SignalHandler(int sig, siginfo_t *info, void *uc) {
    // Allow ourselves to be dumped if the signal is trusted.
    // 如果信号是置信的, 那么我们将进行 dump 错误信息的堆栈
    bool signal_trusted = info->si_code > 0;
    bool signal_pid_trusted = info->si_code == SI_USER ||
                              info->si_code == SI_TKILL;
    if (signal_trusted || (signal_pid_trusted && info->si_pid == getpid())) {
        sys_prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
    }

    // Fill in all the holes in the struct to make Valgrind happy.
    // 为 Crash 的上下文分配内存
    memset(&gCrashContext, 0, sizeof(gCrashContext));
    // 写入 Crash 信号具体信息
    memcpy(&gCrashContext.siginfo, info, sizeof(siginfo_t));
    // 写入 Crash 的上下文信息
    memcpy(&gCrashContext.context, uc, sizeof(ucontext_t));
#if defined(__aarch64__)
    ucontext_t* uc_ptr = (ucontext_t*)uc;
        struct fpsimd_context* fp_ptr =
            (struct fpsimd_context*)&uc_ptr->uc_mcontext.__reserved;
        if (fp_ptr->head.magic == FPSIMD_MAGIC) {
          memcpy(&g_crash_context_.float_state, fp_ptr,
                 sizeof(g_crash_context_.float_state));
        }
#elif !defined(__ARM_EABI__) && !defined(__mips__)
    // FP state is not part of user ABI on ARM Linux.
        // In case of MIPS Linux FP state is already part of ucontext_t
        // and 'float_state' is not a member of CrashContext.
        ucontext_t* uc_ptr = (ucontext_t*)uc;
        if (uc_ptr->uc_mcontext.fpregs) {
          memcpy(&g_crash_context_.float_state, uc_ptr->uc_mcontext.fpregs,
                 sizeof(g_crash_context_.float_state));
        }
#endif
    // 记录当前的线程
    gCrashContext.tid = syscall(__NR_gettid);
    // 处理成功, 将信号置为 SIG_DEF 表示没有用户态信号处理函数, 方便后续让内核处理函数直接执行
    if (dumpStack()) {
        SampleCrashCatcher::restoreKernelDefaultSigHandler(sig);
    }
        // 未处理成功, 则恢复之前的信号处理函数, 通过 tgkill 交由他们执行
    else {
        SampleCrashCatcher::restoreSysSinHandler();
    }
}

void SampleCrashCatcher::installSysSigHandler() {
    // 已经安装过, 就别再次安装了
    if (handlers_installed) {
        return;
    }
    // 1. 保存之前的信号处理函数, 以便恢复
    for (int i = 0; i < gNumHandledSignals; ++i) {
        if (sigaction(gExceptionSignals[i], NULL, &old_handlers[i]) == -1) {
            return;
        }
    }

    // 2. 创建我们自己的信号处理函数
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    // 2.1 初始化 sa.sa_mask 信号集
    sigemptyset(&sa.sa_mask);

    // 2.2 为这个信号处理函数添加它能够处理的信号
    for (int i = 0; i < gNumHandledSignals; ++i) {
        sigaddset(&sa.sa_mask, gExceptionSignals[i]);
    }

    // 2.3 注入信号处理实现
    sa.sa_sigaction = SignalHandler;
    sa.sa_flags = SA_ONSTACK | SA_SIGINFO;

    for (int i = 0; i < gNumHandledSignals; ++i) {
        // 3. 将系统相关的信号处理器替换成我们新建的
        if (sigaction(gExceptionSignals[i], &sa, NULL) == -1) {
            // At this point it is impractical to back out changes, and so failure to
            // install a signal is intentionally ignored.
        }
    }
    handlers_installed = true;
//    ALOGI("===============installSysSigHandler success================");
}

void SampleCrashCatcher::restoreSysSinHandler() {
    // 未安装过不需要恢复
    if (!handlers_installed) {
        return;
    }
    for (int i = 0; i < gNumHandledSignals; ++i) {
        // 将信号处理函数替换成之前的
        if (sigaction(gExceptionSignals[i], &old_handlers[i], NULL) == -1) {
            // 重置系统信号处理器
            restoreKernelDefaultSigHandler(gExceptionSignals[i]);
        }
    }
    handlers_installed = false;
//    ALOGI("===============restoreSysSinHandler success================");
}

void SampleCrashCatcher::restoreKernelDefaultSigHandler(int sig) {
#if defined(__ANDROID__)

    // Android L+ expose signal and sigaction symbols that override the system
    // ones. There is a bug in these functions where a request to set the handler
    // to SIG_DFL is ignored. In that case, an infinite loop is entered as the
    // signal is repeatedly sent to breakpad's signal handler.
    // To work around this, directly call the system's sigaction.
    // 将用户态信号处理函数置空, 下次再发送信号时, 直接由内核态执行
    struct kernel_sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sys_sigemptyset(&sa.sa_mask);
    sa.sa_handler_ = SIG_DFL;
    sa.sa_flags = SA_RESTART;
    sys_rt_sigaction(sig, &sa, NULL, sizeof(kernel_sigset_t));
#else
    signal(sig, SIG_DFL);
#endif
}
