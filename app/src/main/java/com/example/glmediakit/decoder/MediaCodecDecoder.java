package com.example.glmediakit.decoder;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;

public class MediaCodecDecoder {
    private MediaCodec decoder;
    private MediaFormat format;
    private MediaCodec.BufferInfo bufferInfo;
    private boolean isConfigured = false;
    private boolean isStarted = false;
    private boolean isSurfaceMode = false;
    private boolean isAsynMode = false;
    String tag = "MediaCodecVideoDecoder";

    enum decoderStates {Configured, Uninitialized, Error, Flushed, Running, EndOfStream, Released};

    decoderStates state = decoderStates.Uninitialized;

    public MediaCodecDecoder() {
        bufferInfo = new MediaCodec.BufferInfo();
    }
    /**
     * 初始化解码器
     * @param mime_type 解码器类型
     * @param width 视频宽
     * @param height 视频高
     * @param csds 编解码特定数据，如SPS，PPS
     * @param surface 是否使用Surface模式
     * @return 是否成功
     * */
    public boolean initialize(String mime_type, int width, int height,
                        ByteBuffer[] csds, boolean surface) throws IOException {
        // 寻找解码器
        decoder = MediaCodec.createDecoderByType(mime_type);

        // 配置格式
        format = MediaFormat.createVideoFormat(mime_type, width, height);

        // 设置编解码特定数据
        if (csds != null) {
            for (int i = 0; i < csds.length; ++i) {
                if (csds[i] != null) {
                    format.setByteBuffer("csd-" + i, csds[i]);
                }
            }
        }

//        if (surface) {
//
//        }
        decoder.configure(format, null, null, 0);
        state = decoderStates.Configured;

        decoder.start();
        state = decoderStates.Flushed;

        return true;
    }

    /**
     * 提交编码数据包进行解码
     * @param buffer 编码数据
     * @param size 数据大小
     * @param flag 标志位（如BUFFER_FLAG_KEY_FRAME）
     * @return 是否成功提交
     * */
    public boolean pushInputBuffer(ByteBuffer buffer, int size, int flag) {
        if (buffer == null) {
            Log.e(tag, "queueInputBuffer,but buffer is null");
            return false;
        }
        // 同步模式，获取输入缓冲区
        int inputBufferIdx = decoder.dequeueInputBuffer(10000);
        state = decoderStates.Running;
        if (inputBufferIdx >= 0 ) {
            ByteBuffer inputBuffer = decoder.getInputBuffer(inputBufferIdx);
            assert inputBuffer != null;
            inputBuffer.clear();
            inputBuffer.put(buffer);
            decoder.queueInputBuffer(inputBufferIdx, 0, size, 10000, flag);
            return true;
        } else {
           Log.e(tag, "dequeue input buffer failed.");
           return false;
        }
    }

    /**
     * 获取解码数据
     * @param timeoutUs 超时时间
     * @return 获取解码后数据
     * */
    public ByteBuffer getOutputBuffer(long timeoutUs) {
        int outputBufferId = decoder.dequeueOutputBuffer(bufferInfo, timeoutUs);
        if (outputBufferId >= 0) {
            ByteBuffer outputBuffer = decoder.getOutputBuffer(outputBufferId);
            decoder.releaseOutputBuffer(outputBufferId, false);
            return outputBuffer;
        }
        return null;
    }

    public long getPts() {
        return bufferInfo.presentationTimeUs;
    }
    /**
     * 通知EOD
     * */
    public void signalEndOfInputStream() {
        int inputBufferIdx = decoder.dequeueInputBuffer(10000);
        if (inputBufferIdx >= 0) {
            decoder.queueInputBuffer(inputBufferIdx, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
        }
    }

    public void release() {
        if (decoder != null) {
            decoder.stop();
            decoder.release();
            decoder = null;
        }
    }
    public boolean isConfigured() {
        return isConfigured;
    }

    public void setConfigured(boolean configured) {
        isConfigured = configured;
    }

    public boolean isStarted() {
        return isStarted;
    }

    public void setStarted(boolean started) {
        isStarted = started;
    }

    public boolean isSurfaceMode() {
        return isSurfaceMode;
    }

    public void setSurfaceMode(boolean surfaceMode) {
        isSurfaceMode = surfaceMode;
    }

    public boolean isAsynMode() {
        return isAsynMode;
    }

    public void setAsynMode(boolean asynMode) {
        isAsynMode = asynMode;
    }

    public MediaCodec.BufferInfo getBufferInfo() {
        return bufferInfo;
    }
}
