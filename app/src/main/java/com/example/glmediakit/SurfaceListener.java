package com.example.glmediakit;

import android.view.Surface;

public interface SurfaceListener {

    /**
     * 当Surface创建时调用
     *
     * @param surface 创建的Surface对象
     */
    void onSurfaceCreated(Surface surface);

    /**
     * 当Surface尺寸变化时调用
     *
     * @param surface 变化的Surface对象
     * @param width   新的宽度
     * @param height  新的高度
     */
    void onSurfaceChanged(Surface surface, int width, int height);

    /**
     * 当Surface销毁时调用
     */
    void onSurfaceDestroyed();

}