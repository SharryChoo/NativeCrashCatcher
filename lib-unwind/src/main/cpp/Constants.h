//
// Created by SharryChoo on 2020/9/1.
//

#ifndef NATIVECRASHCATCHER_CONSTANTS_H
#define NATIVECRASHCATCHER_CONSTANTS_H

#define LOG_TAG "Native-Crash"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //NATIVECRASHCATCHER_CONSTANTS_H
