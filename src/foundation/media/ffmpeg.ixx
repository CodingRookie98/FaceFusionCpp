/**
 ******************************************************************************
 * @file           : ffmpeg.ixx
 * @brief          : FFmpeg runner module interface
 ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <opencv2/core/mat.hpp>

export module foundation.media.ffmpeg;
export import :remuxer;

namespace foundation::media::ffmpeg {

export class VideoParams {
public:
    explicit VideoParams(const std::string& videoPath);
    VideoParams() = default;

    double frameRate = 0.0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int quality = 80;
    std::string preset = "medium";
    std::string videoCodec = "libx264";

    // Rate Control
    int64_t bitRate = 0;    // 0 = auto/CRF
    int64_t maxBitRate = 0; // VBV max rate
    int bufSize = 0;        // VBV buffer size

    // Format & Structure
    std::string pixelFormat = "yuv420p";
    int gopSize = 12;
    int maxBFrames = 2;

    // Advanced Config
    std::string tune;
    std::string profile;
    std::string level;
    std::string hwAccel;
    int threadCount = 0; // 0 = auto

    std::unordered_map<std::string, std::string> extraOptions;
};

export bool is_video(const std::string& videoPath);
export void extract_frames(const std::string& videoPath, const std::string& outputImagePattern);

export bool compose_video_from_images(const std::string& inputImagePattern,
                                      const std::string& outputVideoPath,
                                      const VideoParams& videoParams);

// ============================================================================
// VideoReader - 流式视频解码器
// ============================================================================
export class VideoReader {
public:
    explicit VideoReader(const std::string& videoPath);
    ~VideoReader();

    // 禁止拷贝
    VideoReader(const VideoReader&) = delete;
    VideoReader& operator=(const VideoReader&) = delete;

    // 允许移动
    VideoReader(VideoReader&&) noexcept;
    VideoReader& operator=(VideoReader&&) noexcept;

    bool open();
    void close();
    [[nodiscard]] bool is_opened() const;

    // 读取下一帧，返回 BGR 格式的 cv::Mat，若到达末尾返回空 Mat
    [[nodiscard]] cv::Mat read_frame();

    // 跳转到指定帧索引 (0-based)
    bool seek(int64_t frame_index);

    // 跳转到指定时间点 (毫秒)
    bool seek_by_time(double timestamp_ms);

    // 视频元数据
    [[nodiscard]] int get_frame_count() const;
    [[nodiscard]] double get_fps() const;
    [[nodiscard]] int get_width() const;
    [[nodiscard]] int get_height() const;
    [[nodiscard]] int64_t get_duration_ms() const;
    [[nodiscard]] double get_current_timestamp_ms() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// VideoWriter - 流式视频编码器
// ============================================================================
export class VideoWriter {
public:
    VideoWriter(const std::string& outputPath, const VideoParams& params);
    ~VideoWriter();

    // 禁止拷贝
    VideoWriter(const VideoWriter&) = delete;
    VideoWriter& operator=(const VideoWriter&) = delete;

    // 允许移动
    VideoWriter(VideoWriter&&) noexcept;
    VideoWriter& operator=(VideoWriter&&) noexcept;

    bool open();
    void close();
    [[nodiscard]] bool is_opened() const;

    // 写入一帧 BGR 格式的图像
    bool write_frame(const cv::Mat& frame);

    // 获取已写入的帧数
    [[nodiscard]] int get_written_frame_count() const;

    // 设置音频来源（从源视频复制音频轨道）
    void set_audio_source(const std::string& sourceVideoPath);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace foundation::media::ffmpeg
