#include <jni.h>
#include <android/log.h>

#include "SampleCrashCatcher.h"
#include "Constants.h"

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

SampleCrashCatcher *g_sample_crash_catcher = NULL;

void sampleCrashCatcher() {
    if (!g_sample_crash_catcher) {
        g_sample_crash_catcher = new SampleCrashCatcher();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_sharry_lib_mynativecrashcatcher_NativeCrashCatcherManager_nativeInit(JNIEnv *env, jclass,
                                                                              jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    sampleCrashCatcher();
    env->ReleaseStringUTFChars(path_, path);
}