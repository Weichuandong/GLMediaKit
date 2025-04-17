package com.example.glmediakit.decoder;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
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
    String tag = "java_MediaCodecVideoDecoder";

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
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
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
    public boolean pushInputBuffer(ByteBuffer buffer, int size, int ts, int flag) {
        if (buffer == null) {
            Log.e(tag, "queueInputBuffer,but buffer is null");
            return false;
        }
        try{
            // 同步模式，获取输入缓冲区
            Log.d(tag, "Decoder state before dequeue: " + state);
            int inputBufferIdx = decoder.dequeueInputBuffer(10000);
            state = decoderStates.Running;
            if (inputBufferIdx >= 0 ) {
                ByteBuffer inputBuffer = decoder.getInputBuffer(inputBufferIdx);
                assert inputBuffer != null;
                inputBuffer.clear();
                inputBuffer.put(buffer);
                decoder.queueInputBuffer(inputBufferIdx, 0, size, ts, flag);
                return true;
            } else {
                Log.e(tag, "dequeue input buffer failed： " + inputBufferIdx);
                // -1: TIMEOUT
                // -2: BUFFER_FLAG_END_OF_STREAM
                // -3: Buffer取不到，可能是错误
                return false;
            }
        } catch (IllegalStateException e) {
            Log.e(tag, "Decoder in illegal state", e);
        } catch (Exception e) {
            Log.e(tag, "Error push Input Buffer", e);
        }
        return false;
    }

    /**
     * 获取解码数据
     * @param timeoutUs 超时时间
     * @return 获取解码后数据
     * */
    public DecodedFrame getOutputBuffer(long timeoutUs) {
        int outputBufferId = decoder.dequeueOutputBuffer(bufferInfo, timeoutUs);
        // 增加详细日志
        Log.d(tag, "dequeueOutputBuffer 返回: " + outputBufferId +
                " flags: " + bufferInfo.flags +
                " size: " + bufferInfo.size);

        if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
            MediaFormat format = decoder.getOutputFormat();
            Log.d(tag, "输出格式变更: " + format);
            // 处理格式变更
            return null;
        } else if (outputBufferId == MediaCodec.INFO_TRY_AGAIN_LATER) {
            Log.d(tag, "暂无输出可用");
            return null;
        } else if (outputBufferId >= 0) {
            ByteBuffer buffer = decoder.getOutputBuffer(outputBufferId);
            return new DecodedFrame(buffer, outputBufferId);
        }
        return null;
    }

    /** 释放输出缓冲区
     * @param outputBufferId 输出缓冲区Id
     * @return 是否成功
     */
    public boolean releaseOutputBuffer(int outputBufferId) {
        decoder.releaseOutputBuffer(outputBufferId, false);
        return true;
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
