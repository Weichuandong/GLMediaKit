package com.example.glmediakit

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.widget.FrameLayout
import android.widget.Toast
import com.example.glmediakit.R

class MainActivity : AppCompatActivity() {

    private lateinit var eglSurfaceView: MediaSurfaceView
    private var currentBitmap: Bitmap? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_main)
        // 1. 获取容器布局
        val container = findViewById<FrameLayout>(R.id.surface_container)

        // 2. 创建EGLSurfaceView实例
        eglSurfaceView = MediaSurfaceView(this)

        // 3. 添加到布局中
        container.addView(eglSurfaceView)

        // 4. 设置加载图片纹理的按钮点击事件
        findViewById<Button>(R.id.btn_load_image).setOnClickListener {
            loadImage()
        }
    }

    // 加载示例图片作为纹理
    private fun loadImage() {
        try {
            // 从资源文件加载图片
            currentBitmap = BitmapFactory.decodeResource(resources, R.drawable.sample_image)

            // 设置到EGLSurfaceView
            eglSurfaceView.setImage(currentBitmap)

            // Kotlin中不需要手动回收bitmap，GC会在适当时机回收bitmap
        } catch (e: Exception) {
            Toast.makeText(this, "加载图片失败: ${e.message}", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onPause() {
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onDestroy() {
        eglSurfaceView.release()
        super.onDestroy()
    }

    companion object {
        init {
            System.loadLibrary("GLMediaKit")
        }
    }
}