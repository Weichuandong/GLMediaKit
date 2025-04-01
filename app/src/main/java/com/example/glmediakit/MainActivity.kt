package com.example.glmediakit

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.provider.MediaStore
import android.util.Log
import android.widget.Button
import android.widget.FrameLayout
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.io.File
import java.io.FileOutputStream
import java.io.OutputStream
import androidx.activity.result.contract.ActivityResultContracts

class MainActivity : AppCompatActivity() {

    private lateinit var eglSurfaceView: MediaSurfaceView
    private lateinit var player: Player

    // 视频文件选择请求码
    private val REQUEST_VIDEO_FILE = 1001

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_main)
        // 获取容器布局
        val container = findViewById<FrameLayout>(R.id.surface_container)

        // 创建EGLSurfaceView实例并添加到布局中
        eglSurfaceView = MediaSurfaceView(this)
        container.addView(eglSurfaceView)

        // 初始化Player相关

        player = Player()

        eglSurfaceView.setSurfaceListener(player)

        findViewById<Button>(R.id.playback).setOnClickListener {
            player.playback();
        }

        findViewById<Button>(R.id.pause).setOnClickListener {
            player.pause();
        }

        findViewById<Button>(R.id.pkVideo).setOnClickListener {
            openVideoPicker();
        }
    }
    // 在 Activity 中
    private val videoPickerLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        uri ?: return@registerForActivityResult
        handleVideoUri(uri)
    }

    // 打开视频选择器
    fun openVideoPicker() {
        videoPickerLauncher.launch("video/*")
    }

    private fun handleVideoUri(uri: Uri) {
        try {
            contentResolver.openFileDescriptor(uri, "r")?.use {
                // 将文件描述符转换为路径后传递给 C++ 播放器
                val path = getVideoPathFromUri(this, uri)
                player.prepare(path)
                Toast.makeText(this, "已选择视频: $uri", Toast.LENGTH_SHORT).show()
            } ?: run {
                Toast.makeText(this, "无法访问文件内容", Toast.LENGTH_SHORT).show()
            }
        } catch (e: Exception) {
            Toast.makeText(this, "错误: ${e.message}", Toast.LENGTH_SHORT).show()
        }
    }

    fun getVideoPathFromUri(context: Context, uri: Uri): String? {
        // 先尝试通过传统方法获取路径
        var filePath = getPathFromUri(context, uri)

        // 如果获取路径失败，复制文件到缓存目录
        if (filePath == null) {
            try {
                // 创建目标文件
                val fileName = "video_" + System.currentTimeMillis() + ".mp4"
                val destinationFile = File(context.cacheDir, fileName)

                // 开始复制
                val inputStream = context.contentResolver.openInputStream(uri)
                val outputStream: OutputStream = FileOutputStream(destinationFile)

                val buffer = ByteArray(4 * 1024) // 4kb buffer
                var bytesRead: Int
                while ((inputStream!!.read(buffer).also { bytesRead = it }) != -1) {
                    outputStream.write(buffer, 0, bytesRead)
                }

                // 确保流被完全写入
                outputStream.flush()

                // 关闭流
                inputStream.close()
                outputStream.close()

                filePath = destinationFile.absolutePath
                Log.d("VideoPlayer", "文件已复制到: $filePath")
            } catch (e: java.lang.Exception) {
                Log.e("VideoPlayer", "复制文件失败", e)
                return null
            }
        }
        return filePath
    }

    // 尝试常规方法获取路径
    private fun getPathFromUri(context: Context, uri: Uri): String? {
        try {
            val projection = arrayOf(MediaStore.Video.Media.DATA)
            val cursor = context.contentResolver.query(uri, projection, null, null, null)
            if (cursor != null) {
                val column_index = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA)
                if (cursor.moveToFirst()) {
                    val path = cursor.getString(column_index)
                    cursor.close()
                    return path
                }
                cursor.close()
            }
        } catch (e: java.lang.Exception) {
            Log.e("VideoPlayer", "获取路径失败", e)
        }
        return null
    }

    override fun onPause() {
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onDestroy() {
        player.release()
        super.onDestroy()
    }

    companion object {
        init {
            System.loadLibrary("GLMediaKit")
        }
    }
}