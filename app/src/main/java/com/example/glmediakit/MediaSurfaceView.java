package com.example.glmediakit;

import android.content.Context;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import androidx.annotation.NonNull;

public class MediaSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private SurfaceListener mListener = null;

    public MediaSurfaceView(Context context) {
        this(context, null);
    }

    public MediaSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        System.loadLibrary("GLMediaKit");

        // 注册Surface回调
        getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        if (mListener != null) {
            mListener.onSurfaceCreated(holder.getSurface());
        }
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        if (mListener != null) {
            mListener.onSurfaceChanged(holder.getSurface(), width, height);
        }
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        if (mListener != null) {
            mListener.onSurfaceDestroyed();
        }
    }

    /**
     *  添加Surface监听者
     *  @param listener 监听者
     */
    public void setSurfaceListener(SurfaceListener listener) {
        mListener = listener;

        // 如果Surface已经存在，直接通知监听者
        if (getHolder().getSurface() != null && getHolder().getSurface().isValid()) {
            listener.onSurfaceCreated(getHolder().getSurface());
        }
    }

    public void moveSurfaceListener() {
        mListener = null;
    }
}
