/**
 * @file ffmpeg_raii.h
 * @brief RAII wrappers for FFmpeg resources
 * @note This is an internal header, not exported via modules
 */
#pragma once

#include <memory>

// Forward declarations for FFmpeg types
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace foundation::media::ffmpeg::raii {

// Custom deleters
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) const;
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const;
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) const;
};

struct AVPacketDeleter {
    void operator()(AVPacket* packet) const;
};

struct SwsContextDeleter {
    void operator()(SwsContext* ctx) const;
};

// Type aliases
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

// Helper functions to create RAII wrappers
AVFormatContextPtr make_format_context();
AVCodecContextPtr make_codec_context(const AVCodecContext* src);
AVFramePtr make_frame();
AVPacketPtr make_packet();
SwsContextPtr make_sws_context(int srcW, int srcH, int srcFormat, int dstW, int dstH, int dstFormat,
                               int flags, SwsContext* src = nullptr);

} // namespace foundation::media::ffmpeg::raii
