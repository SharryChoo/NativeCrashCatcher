package com.sharry.lib.mynativecrashcatcher;


public class NativeCrashCatcherManager {

    static {
        System.loadLibrary("unwind-catcher");
    }

    public static void init(String path) {
        nativeInit(path);
    }

    private static native void nativeInit(String path);

}
