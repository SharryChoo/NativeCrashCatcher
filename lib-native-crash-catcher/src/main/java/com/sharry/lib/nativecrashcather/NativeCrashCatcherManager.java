package com.sharry.lib.nativecrashcather;


public class NativeCrashCatcherManager {

    static {
        System.loadLibrary("native-crash-catcher");
    }

    public static void init(String path) {
        nativeInit(path);
    }

    private static native void nativeInit(String path);

}
