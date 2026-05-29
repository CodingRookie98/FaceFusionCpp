/**
 * @file ffmpeg_raii.cpp
 * @brief Implementation of RAII wrappers for FFmpeg resources
 */
#include "ffmpeg_raii.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace foundation::media::ffmpeg::raii {

// Deleter implementations
void AVFormatContextDeleter::operator()(AVFormatContext* ctx) const {
    if (ctx) avformat_close_input(&ctx);
}

void AVCodecContextDeleter::operator()(AVCodecContext* ctx) const {
    if (ctx) avcodec_free_context(&ctx);
}

void AVFrameDeleter::operator()(AVFrame* frame) const {
    if (frame) av_frame_free(&frame);
}

void AVPacketDeleter::operator()(AVPacket* packet) const {
    if (packet) av_packet_free(&packet);
}

void SwsContextDeleter::operator()(SwsContext* ctx) const {
    if (ctx) sws_freeContext(ctx);
}

// Helper function implementations
AVFormatContextPtr make_format_context() {
    return AVFormatContextPtr(avformat_alloc_context());
}

AVCodecContextPtr make_codec_context(const AVCodecContext* src) {
    // This is a placeholder - actual implementation depends on use case
    // In practice, you'd typically get codec context from format context
    return nullptr;
}

AVFramePtr make_frame() {
    return AVFramePtr(av_frame_alloc());
}

AVPacketPtr make_packet() {
    return AVPacketPtr(av_packet_alloc());
}

SwsContextPtr make_sws_context(int srcW, int srcH, int srcFormat, int dstW, int dstH, int dstFormat,
                               int flags, SwsContext* src) {
    SwsContext* ctx =
        sws_getContext(srcW, srcH, static_cast<AVPixelFormat>(srcFormat), dstW, dstH,
                       static_cast<AVPixelFormat>(dstFormat), flags, nullptr, nullptr, nullptr);
    return SwsContextPtr(ctx);
}

} // namespace foundation::media::ffmpeg::raii
