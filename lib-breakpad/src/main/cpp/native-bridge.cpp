#include <jni.h>
#include <android/log.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"

#define LOG_TAG "Native-Crash"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

bool DumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                  void *context,
                  bool succeeded) {
    ALOGE("===============Native-Cash================");
    ALOGE("Dump path: %s\n", descriptor.path());
    return succeeded;
}

void breakpadCrashCatcher(const char *path) {
    google_breakpad::MinidumpDescriptor descriptor(path);
    static google_breakpad::ExceptionHandler eh(descriptor, NULL, DumpCallback, NULL, true, -1);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sharry_lib_breakpadnativecrashcatcher_NativeCrashCatcherManager_nativeInit(JNIEnv *env, jclass type,
                                                                           jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    // 使用 breakpad 进行 native crash 的捕获
    breakpadCrashCatcher(path);
    env->ReleaseStringUTFChars(path_, path);

}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}