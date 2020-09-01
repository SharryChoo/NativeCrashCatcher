package com.sharry.lib.breakpadnativecrashcatcher;


public class NativeCrashCatcherManager {

    static {
        System.loadLibrary("breakpad-native-crash-catcher");
    }

    public static void init(String path) {
        nativeInit(path);
    }

    private static native void nativeInit(String path);

}
