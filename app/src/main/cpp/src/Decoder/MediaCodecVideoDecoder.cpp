//
// Created by Weichuandong on 2025/4/8.
//

#include "Decoder/MediaCodecVideoDecoder.h"

MediaCodecVideoDecoder::MediaCodecVideoDecoder() :
    mediaCodecDecoderWrapper(std::make_unique<MediaCodecDecoderWrapper>()),
    ready(false),
    mHeight(0),
    mWidth(0)
{

}

int MediaCodecVideoDecoder::SendPacket(const std::shared_ptr<IMediaPacket> &packet) {
    // packet中的数据
    if (!packet) {
        LOGE("packet is null");
        return false;
    }

    const uint8_t* data = packet->getData();
    int size = packet->getSize();
    // 从AVCC格式转为Annex-B格式
//    std::vector<uint8_t> AVCCData(data, data + size);
//    std::vector<uint8_t> AnnexBData;
//    if (isIDRFrame(const_cast<AVPacket *>(packet->asAVPacket()))) {
//        if (decoderConfig.extraData.find("sps") == decoderConfig.extraData.end() ||
//            decoderConfig.extraData.find("pps") == decoderConfig.extraData.end()) {
//            LOGE("don't hava sps, pps");
//            return -1;
//        }
//        AnnexBData = convertIDRAVCCToAnnexB(AVCCData, decoderConfig.extraData.at("sps"), decoderConfig.extraData.at("pps"));
//    } else {
//        AnnexBData = convertAVCCToAnnexB(AVCCData);
//    }

    LOGI("SendPacket params : size = %d, ts = %ld", size, packet->getPts());
    if (mediaCodecDecoderWrapper->pushEncodedData(data, size, packet->getPts(), 0)) {
        return 0;
    } else {
        return -1;
    }
}

int MediaCodecVideoDecoder::ReceiveFrame(std::shared_ptr<IMediaFrame> &frame) {
    std::vector<uint8_t> data;
    size_t dataSize;
    int64_t pts;
    int bufferId;
    bool rst = mediaCodecDecoderWrapper->getDecodedData(data, dataSize, pts, bufferId);
    if (!rst || !frame) {
        LOGE("getDecodedData failed");
        return -1;
    }
    frame->createFromYUV420P(data.data(), mWidth, mHeight, pts);
    // 释放buffer
    rst = mediaCodecDecoderWrapper->releaseOutputBuffer(bufferId);

    if (rst) return 0;

    return -1;
}

bool MediaCodecVideoDecoder::isReadying() {
    return ready;
}

bool MediaCodecVideoDecoder::configure(const DecoderConfig &config) {
    decoderConfig = config;
    // 调用wrapper的init方法
    // 根据config获取参数
    if (decoderConfig.extraData.find("sps") == decoderConfig.extraData.end() ||
            decoderConfig.extraData.find("pps") == decoderConfig.extraData.end()) {
        LOGE("config not have sps and pps");
        return false;
    }
    mWidth = decoderConfig.width;
    mHeight = decoderConfig.height;
    format = decoderConfig.format;
    const auto& sps = decoderConfig.extraData.at("sps");
    const auto& pps = decoderConfig.extraData.at("pps");
    return mediaCodecDecoderWrapper->init(decoderConfig.type, mWidth, mHeight,
                                   sps.data(), sps.size(), pps.data(), pps.size());

}

int MediaCodecVideoDecoder::getWidth() {
    return mWidth;
}

int MediaCodecVideoDecoder::getHeight() {
    return mHeight;
}

PixFormat MediaCodecVideoDecoder::getPixFormat() {
    return PixFormat::YUV420P;
}

std::vector<uint8_t> MediaCodecVideoDecoder::convertAVCCToAnnexB(std::vector<uint8_t> &src) {
    // 将AVCC格式的数据转换为Annex-B格式
    if (src.size() < 4) {
        LOGE("src data format is wrong");
        return src;
    }

    int offset = 0;
    std::vector<uint8_t> startCode = {0x00, 0x00, 0x00, 0x01};

    // 先计算输入大小，减少输出vector的空间分配次数
    int outputSize = 0;
    while (offset + 4 <= src.size()) {
        // 读取Nalu单元长度 （4字节 大端数据）
        int nalLength = (src[offset] << 24 | src[offset + 1] << 16 |
                         src[offset + 2] << 8 | src[offset + 3]);

        if (nalLength <= 0 || offset + nalLength > src.size()) {
            LOGE("this nal length is abnormal");
            break;
        }
        offset += 4;
        offset += nalLength;

        outputSize += offset;
        outputSize += nalLength;
    }
    std::vector<uint8_t> dst;
    dst.reserve(outputSize);

    // 真正组合数据
    offset = 0;
    while(offset + 4 <= src.size()) {
        int nalLength = (src[offset] << 24 | src[offset + 1] << 16 |
                         src[offset + 2] << 8 | src[offset + 3]);

        if (nalLength <= 0 || offset + nalLength > src.size()) {
            LOGE("this nal length is abnormal");
            break;
        }
        offset += 4;

        dst.insert(dst.end(), startCode.begin(), startCode.end());
        dst.insert(dst.end(), src.begin() + offset, src.begin() + offset + nalLength);

        offset += nalLength;
    }

    return dst;
}

bool MediaCodecVideoDecoder::isIDRFrame(AVPacket *packet) {
    if ((packet->flags & AV_PKT_FLAG_KEY) != 0) {
        return true;
    }
    return false;
}

std::vector<uint8_t>
MediaCodecVideoDecoder::convertIDRAVCCToAnnexB(std::vector<uint8_t> &src, std::vector<uint8_t> &sps,
                                               std::vector<uint8_t> &pps) {
    if (src.size() < 4) {
        LOGE("src data format is wrong");
        return src;
    }

    int offset = 0;
    std::vector<uint8_t> startCode = {0x00, 0x00, 0x00, 0x01};

    // 先计算输入大小，减少输出vector的空间分配次数
    int outputSize = (4 + sps.size() + 4 + pps.size());

    while (offset + 4 <= src.size()) {
        // 读取Nalu单元长度 （4字节 大端数据）
        int nalLength = (src[offset] << 24 | src[offset + 1] << 16 |
                         src[offset + 2] << 8 | src[offset + 3]);

        if (nalLength <= 0 || offset + nalLength > src.size()) {
            LOGE("this nal length is abnormal");
            break;
        }
        offset += 4;
        offset += nalLength;
        outputSize += offset;
        outputSize += nalLength;
    }

    std::vector<uint8_t> dst;
    dst.reserve(outputSize);

    dst.insert(dst.end(), startCode.begin(), startCode.end());
    dst.insert(dst.end(), sps.begin(), sps.end());
    dst.insert(dst.end(), startCode.begin(), startCode.end());
    dst.insert(dst.end(), pps.begin(), pps.end());

    offset = 0;
    while (offset + 4 <= src.size()) {
        // 读取Nalu单元长度 （4字节 大端数据）
        int nalLength = (src[offset] << 24 | src[offset + 1] << 16 |
                         src[offset + 2] << 8 | src[offset + 3]);

        if (nalLength <= 0 || offset + nalLength > src.size()) {
            LOGE("this nal length is abnormal");
            break;
        }

        offset += 4;

        dst.insert(dst.end(), startCode.begin(), startCode.end());
        dst.insert(dst.end(), src.begin() + offset, src.begin() + offset + nalLength);

        offset += nalLength;
    }
    return dst;
}
