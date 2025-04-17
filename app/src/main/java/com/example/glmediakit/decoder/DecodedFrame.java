package com.example.glmediakit.decoder;

import java.nio.ByteBuffer;

public class DecodedFrame {
    private final ByteBuffer buffer;
    private final int outputBufferId;

    public DecodedFrame(ByteBuffer buffer, int outputBufferId) {
        this.buffer = buffer;
        this.outputBufferId = outputBufferId;
    }

    public ByteBuffer getBuffer() { return buffer; }

    public int getOutputBufferId() { return outputBufferId; }
}
