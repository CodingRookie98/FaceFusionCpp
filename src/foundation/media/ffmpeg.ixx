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

namespace foundation::media::ffmpeg {
export enum Audio_Codec { Codec_AAC, Codec_MP3, Codec_OPUS, Codec_VORBIS, Codec_UNKNOWN };

export class VideoPrams {
public:
    // Initialize the `frameRate`, `width`, and `height` member variables.
    explicit VideoPrams(const std::string& videoPath);

    double frameRate;
    unsigned int width;
    unsigned int height;
    unsigned int quality;
    std::string preset;
    std::string videoCodec;
};

export std::vector<std::string> child_process(const std::string& command);
export bool is_video(const std::string& videoPath);
export bool is_audio(const std::string& videoPath);
export void extract_frames(const std::string& videoPath, const std::string& outputImagePattern);
export void extract_audios(const std::string& videoPath, const std::string& outputDir,
                           const Audio_Codec& audioCodec = Codec_AAC);
export bool add_audios_to_video(const std::string& videoPath,
                                const std::vector<std::string>& audioPaths,
                                const std::string& outputVideoPath);

// no audio output
export bool cut_video_into_segments(const std::string& videoPath, const std::string& outputPath,
                                    const unsigned int& segmentDuration,
                                    const std::string& outputPattern);

export bool concat_video_segments(const std::vector<std::string>& videoSegmentsPaths,
                                  const std::string& outputVideoPath, const VideoPrams& videoPrams);

export std::unordered_map<unsigned int, std::string> get_audio_streams_index_and_codec(
    const std::string& videoPath);

export std::unordered_set<std::string> filter_video_paths(
    const std::unordered_set<std::string>& filePaths);
export std::unordered_set<std::string> filter_audio_paths(
    const std::unordered_set<std::string>& filePaths);

export bool images_to_video(const std::string& inputImagePattern,
                            const std::string& outputVideoPath, const VideoPrams& videoPrams);
export Audio_Codec get_audio_codec(const std::string& codec);

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
    VideoWriter(const std::string& outputPath, const VideoPrams& params);
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
