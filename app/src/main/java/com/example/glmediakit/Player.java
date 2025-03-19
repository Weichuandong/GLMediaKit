package com.example.glmediakit;

import android.util.Log;
import android.view.Surface;

public class Player implements SurfaceListener {

    private static final String TAG = "Player";

    /**
     * 播放器状态
     */
    public enum PlayerState {
        INIT,           // 初始状态
        PREPARED,       // 已准备好
        PLAYING,        // 正在播放
        SEEKING,        //
        PAUSED,         // 已暂停
        STOPPED,        // 已停止
        COMPLETED,      // 播放完成
        ERROR,          // 错误状态
    }


    private long nativeHandle = 0;

    private native long nativeInit();

    private native void nativePrepare(long handle, String filePath);

    private native void nativePlayback(long handle);

    private native void nativePause(long handle);

    private native void nativeResume(long handle);

    private native void nativeStop(long handle);

    private native void nativeRelease(long handle);

    private native void nativeSurfaceCreate(long handle, Surface surface);

    private native void nativeSurfaceChanged(long handle, int width, int heigth);

    private native void nativeSurfaceDestroyed(long handle);

    private native int nativeGetPlayerState(long handle);

    public Player() {
        System.loadLibrary("GLMediaKit");

        nativeHandle = nativeInit();
    }

    public void prepare(String filePath){
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativePrepare(nativeHandle, filePath);
    }

    public void playback(){
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativePlayback(nativeHandle);
    }

    public void pause() {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativePause(nativeHandle);
    }

    public void resume() {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativeResume(nativeHandle);
    }

    public void stop() {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativeStop(nativeHandle);
    }

    public void release() {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativeRelease(nativeHandle);
    }

    public int getPlayerState() {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
            return -1;
        }
        return nativeGetPlayerState(nativeHandle);
    }

    @Override
    public void onSurfaceCreated(Surface surface) {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativeSurfaceCreate(nativeHandle, surface);
    }

    @Override
    public void onSurfaceChanged(Surface surface, int width, int height) {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativeSurfaceChanged(nativeHandle, width, height);
    }

    @Override
    public void onSurfaceDestroyed() {
        if (nativeHandle == 0) {
            Log.e(TAG, "Player don't initialized");
        }
        nativeSurfaceDestroyed(nativeHandle);
    }

}
