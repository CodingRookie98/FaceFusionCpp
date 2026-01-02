/**
 ******************************************************************************
 * @file           : types.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 2025/12/14
 ******************************************************************************
 */
module;

export module types;

export namespace ffc {

enum class ImageFormat {
    PNG,
    JPG,
    JPEG,
};

enum class VideoEncoder {
    libx264,
    libx265,
    libvpx_vp9,
    h264_nvenc,
    hevc_nvenc,
    h264_amf,
    hevc_amf,
};

enum class VideoPreset {
    ultrafast,
    superfast,
    veryfast,
    faster,
    fast,
    medium,
    slow,
    slower,
    veryslow,
};
/**
 * @brief 音频编码器
 */
enum class AudioEncoder {
    aac,
    libmp3lame,
    libopus,
    libvorbis,
};
} // namespace ffc