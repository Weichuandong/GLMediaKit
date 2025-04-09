//
// Created by Weichuandong on 2025/4/8.
//

#include "Decoder/MediaCodecVideoDecoder.h"

MediaCodecVideoDecoder::MediaCodecVideoDecoder() {

}

int MediaCodecVideoDecoder::SendPacket(const std::shared_ptr<IMediaPacket> &packet) {
    return 0;
}

int MediaCodecVideoDecoder::ReceiveFrame(std::shared_ptr<IMediaFrame> &frame) {
    return 0;
}

bool MediaCodecVideoDecoder::isReadying() {
    return false;
}

bool MediaCodecVideoDecoder::configure(const DecoderConfig &config) {
    return false;
}

int MediaCodecVideoDecoder::getWidth() {
    return 0;
}

int MediaCodecVideoDecoder::getHeight() {
    return 0;
}

PixFormat MediaCodecVideoDecoder::getPixFormat() {
    return PixFormat::YUV420P;
}
