//
// Created by Weichuandong on 2025/4/7.
//

#ifndef GLMEDIAKIT_FFMPEGPACKET_HPP
#define GLMEDIAKIT_FFMPEGPACKET_HPP

#include "interface/IMediaData.h"
#include <memory>

class FFmpegPacket : public IMediaPacket {
public:
    explicit FFmpegPacket(AVPacket* data) :
        packet(data) {}

    virtual ~FFmpegPacket() {
//        if (packet) {
//            av_packet_free(&packet);
//        }
    }

    const uint8_t* getData() const override { return packet->data; };

    virtual const size_t getSize() const override { return packet->size; }

    virtual const int64_t getPts() const override { return packet->pts; }

    virtual const AVPacket* asAVPacket() const override { return packet; }

    static std::shared_ptr<FFmpegPacket> fromAVPacket(AVPacket* packet) {
        AVPacket* clone = av_packet_clone(packet);
        if (!clone) {
            return nullptr;
        }
        av_packet_unref(packet);
        return std::shared_ptr<FFmpegPacket>(std::make_shared<FFmpegPacket>(clone));
    }
private:
    AVPacket* packet;
};

class FFmpegPacketFactory {
public:
    static std::shared_ptr<FFmpegPacket> create(AVPacket* packet) {
        return FFmpegPacket::fromAVPacket(packet);
    }
};
#endif //GLMEDIAKIT_FFMPEGPACKET_HPP
