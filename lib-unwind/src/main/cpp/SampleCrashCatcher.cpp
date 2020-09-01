#include <csignal>
#include <cstring>
#include <algorithm>
#include <dlfcn.h>
#include <inttypes.h>
#include "SampleCrashCatcher.h"
#include "Constants.h"
#include "linux_syscall_support.h"
#include "unwind.h"

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
        ALOGI("===============installAlterStack success================");
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
    ALOGI("===============restoreAlterStack success================");
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

_Unwind_Reason_Code unwindCallback(_Unwind_Context *cxt, void *arg) {
    BackTrace *backtrace = (BackTrace *) arg;
    // 获取当前栈帧的中存放 pc 寄存器历史值的地址
    uintptr_t cur_sf_pc = _Unwind_GetIP(cxt);
    // 获取当前栈帧的中存放 sp 寄存器历史值的地址
    uintptr_t cur_sf_sp = _Unwind_GetCFA(cxt);
    // 当前栈帧的 pc 与 sp 和上一个栈帧相等 || 已经回溯到了我们所需的最大深度, 则结束这次栈回溯
    if ((backtrace->frame_num > 0 && cur_sf_pc == backtrace->prev_pc &&
         cur_sf_sp == backtrace->prev_sp)|| backtrace->frame_num > MAX_BACKTRACE_DEPTH) {
        return _URC_END_OF_STACK;
    }
    // 找寻回溯起始位置
    if (backtrace->found_stack == 0) {
        // 定位 Crash 时的目标栈帧
        if (
            // 确保存在栈帧中存在 pc 值
                backtrace->crash_sf_pc >= sizeof(uintptr_t)
                // 确保当前栈帧中存放 pc 值的地址, 与 crash 时的栈帧存放 pc 值的地址在一定的误差之内
                && (cur_sf_pc <= backtrace->crash_sf_pc + sizeof(uintptr_t) &&
                    cur_sf_pc >= backtrace->crash_sf_pc - sizeof(uintptr_t))) {
            // 说明当前回溯到了 Crash 所在的栈帧
            backtrace->found_stack = 1;
        } else {
            return _URC_NO_REASON;
        }
    }
    // 打印回溯到的数据
    Dl_info info;
    dladdr((void *) cur_sf_pc, &info);
    ALOGW(
            "  #%d pc %08u %s (%s + %u)",
            backtrace->frame_num,                       // 栈的 num
            cur_sf_pc - (uintptr_t) info.dli_fbase,     // 减去 so 库的起始地址, 获得 crash 函数代码的相对地址
            info.dli_fname,                             // so 库名称
            info.dli_sname,                             // 最接近 cur_sf_pc 的所指定的符号(即函数名)
            cur_sf_pc - (uintptr_t) info.dli_saddr      // cur_sf_pc 地址与 info.dli_sname 地址的偏移量 (即与代码段函数地址的偏移量)
    );
    // 更新数据, 准备下一层的回溯
    backtrace->frame_num++;
    // 若是达到了我们所需的最大深度, 也直接 return
    if (backtrace->frame_num > MAX_BACKTRACE_DEPTH) {
        return _URC_END_OF_STACK;
    }
    // 记录当前栈帧中 sp 的数据
    backtrace->prev_sp = cur_sf_sp;
    // 记录当前栈帧中 pc 的数据
    backtrace->prev_pc = cur_sf_pc;
    return _URC_NO_REASON;
}

bool dumpStack(siginfo_t *info, void *uc) {
    if (!info || !uc) {
        return false;
    }
    // print brief info.
    int tid = syscall(__NR_gettid);
    // 获取崩溃时相关寄存器的值
    ucontext_t *uc_ptr = (ucontext_t *) uc;
    BackTrace backTrace;
    backTrace.frame_num = 0;
    backTrace.found_stack = 0;
    backTrace.prev_sp = 0;
    backTrace.prev_pc = 0;
    backTrace.crash_sf_pc = uc_ptr->uc_mcontext.arm_pc;
    ALOGW(
            "Native Crash ===> tid: %d, signal: %d, code: %d, error: %d",
            tid,
            info->si_signo,
            info->si_code,
            info->si_errno
    );
    _Unwind_Backtrace(unwindCallback, &backTrace);
    return true;
}

// 1. 第一个参数为信号值
// 2. 第二个参数为信号的一些具体信息
// 3. 第三个参数为一些上下文信息, 包括崩溃时的 pc 值
void SignalHandler(int sig, siginfo_t *info, void *uc) {
    // Allow ourselves to be dumped if the signal is trusted.
    // 如果信号是置信的, 那么我们将进行 dump 错误信息的堆栈
    bool signal_trusted = info->si_code > 0;
    bool signal_pid_trusted = info->si_code == SI_USER ||
                              info->si_code == SI_TKILL;
    if (signal_trusted || (signal_pid_trusted && info->si_pid == getpid())) {
        sys_prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
    }
    // 记录当前的线程
    // 处理成功, 将信号置为 SIG_DEF 表示没有用户态信号处理函数, 方便后续让内核处理函数直接执行
    if (dumpStack(info, uc)) {
        SampleCrashCatcher::restoreKernelDefaultSigHandler(sig);
    }
        // 未处理成功, 则恢复之前的信号处理函数, 通过 tgkill 交由他们执行
    else {
        SampleCrashCatcher::restoreSysSinHandler();
    }
    // 重新发送这个信号, 转发给之前的信号处理函数处理
    if (info->si_code <= 0 || sig == SIGABRT) {
        // 重新调用 tgkill 发送这个信号, 交由之前的信号处理函数进行处理
        if (sys_tgkill(getpid(), syscall(__NR_gettid), sig) < 0) {
            // If we failed to kill ourselves (e.g. because a sandbox disallows us
            // to do so), we instead resort to terminating our process. This will
            // result in an incorrect exit code.
            _exit(1);
        }
    } else {
        // This was a synchronous signal triggered by a hard fault (e.g. SIGSEGV).
        // No need to reissue the signal. It will automatically trigger again,
        // when we return from the signal handler.
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
    ALOGI("===============installSysSigHandler success================");
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
    ALOGI("===============restoreSysSinHandler success================");
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
    ALOGI("===============restore %d kernel default sig handler success================", sig);
}
