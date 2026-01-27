/**
 * @file ffmpeg.ixx
 * @brief Video processing module using FFmpeg
 * @author CodingRookie
 * @date 2026-01-27
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

/**
 * @brief Configuration parameters for video encoding and metadata
 */
export class VideoParams {
public:
    /**
     * @brief Initialize video parameters from an existing video file
     * @param videoPath Path to the template video file
     */
    explicit VideoParams(const std::string& videoPath);
    VideoParams() = default;

    double frameRate = 0.0;             ///< Video frame rate (FPS)
    unsigned int width = 0;             ///< Video width in pixels
    unsigned int height = 0;            ///< Video height in pixels
    unsigned int quality = 80;          ///< Encoding quality (0-100)
    std::string preset = "medium";      ///< FFmpeg encoding preset (e.g., fast, medium, slow)
    std::string videoCodec = "libx264"; ///< Name of the video codec to use

    // Rate Control
    std::int64_t bitRate = 0;    ///< Target bit rate in bits/s (0 = auto/CRF)
    std::int64_t maxBitRate = 0; ///< VBV maximum bit rate
    int bufSize = 0;             ///< VBV buffer size

    // Format & Structure
    std::string pixelFormat = "yuv420p"; ///< Output pixel format
    int gopSize = 12;                    ///< Group of Pictures size
    int maxBFrames = 2;                  ///< Maximum number of B-frames

    // Advanced Config
    std::string tune;    ///< FFmpeg tune setting
    std::string profile; ///< H.264 profile
    std::string level;   ///< H.264 level
    std::string hwAccel; ///< Hardware acceleration strategy
    int threadCount = 0; ///< Number of encoding threads (0 = auto)

    std::unordered_map<std::string, std::string>
        extraOptions; ///< Additional codec-specific options
};

/**
 * @brief Check if a file is a valid video format
 * @param videoPath Path to the file
 * @return true if valid, false otherwise
 */
export bool is_video(const std::string& videoPath);

/**
 * @brief Extract all frames from a video file as images
 * @param videoPath Path to the source video
 * @param outputImagePattern Pattern for output images (e.g., "frame_%04d.png")
 */
export void extract_frames(const std::string& videoPath, const std::string& outputImagePattern);

/**
 * @brief Compose a video from a sequence of images
 * @param inputImagePattern Pattern matching the input images
 * @param outputVideoPath Path to the resulting video file
 * @param videoParams Encoding parameters
 * @return true if success, false otherwise
 */
export bool compose_video_from_images(const std::string& inputImagePattern,
                                      const std::string& outputVideoPath,
                                      const VideoParams& videoParams);

/**
 * @brief Stream-based video decoder (Reader)
 */
export class VideoReader {
public:
    /**
     * @brief Construct a new Video Reader
     * @param videoPath Path to the source video file
     */
    explicit VideoReader(const std::string& videoPath);
    ~VideoReader();

    VideoReader(const VideoReader&) = delete;
    VideoReader& operator=(const VideoReader&) = delete;

    VideoReader(VideoReader&&) noexcept;
    VideoReader& operator=(VideoReader&&) noexcept;

    /**
     * @brief Open the video file for reading
     * @return true if successful, false otherwise
     */
    bool open();

    /**
     * @brief Close the video file and release resources
     */
    void close();

    /**
     * @brief Check if the reader is currently open
     */
    [[nodiscard]] bool is_opened() const;

    /**
     * @brief Read the next frame from the video
     * @return BGR format cv::Mat, or empty Mat if at EOF or error
     */
    [[nodiscard]] cv::Mat read_frame();

    /**
     * @brief Jump to a specific frame index
     * @param frame_index 0-based frame index
     * @return true if seek succeeded
     */
    bool seek(std::int64_t frame_index);

    /**
     * @brief Jump to a specific time point
     * @param timestamp_ms Timestamp in milliseconds
     * @return true if seek succeeded
     */
    bool seek_by_time(double timestamp_ms);

    // Video Metadata
    [[nodiscard]] int get_frame_count() const;             ///< Total number of frames
    [[nodiscard]] double get_fps() const;                  ///< Video frame rate
    [[nodiscard]] int get_width() const;                   ///< Video width in pixels
    [[nodiscard]] int get_height() const;                  ///< Video height in pixels
    [[nodiscard]] std::int64_t get_duration_ms() const;    ///< Total duration in milliseconds
    [[nodiscard]] double get_current_timestamp_ms() const; ///< Current playback position in ms

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Stream-based video encoder (Writer)
 */
export class VideoWriter {
public:
    /**
     * @brief Construct a new Video Writer
     * @param outputPath Path where the output video will be saved
     * @param params Encoding configuration
     */
    VideoWriter(const std::string& outputPath, const VideoParams& params);
    ~VideoWriter();

    VideoWriter(const VideoWriter&) = delete;
    VideoWriter& operator=(const VideoWriter&) = delete;

    VideoWriter(VideoWriter&&) noexcept;
    VideoWriter& operator=(VideoWriter&&) noexcept;

    /**
     * @brief Open the output file for writing
     * @return true if successful
     */
    bool open();

    /**
     * @brief Close the file and finalize encoding
     */
    void close();

    /**
     * @brief Check if the writer is currently open
     */
    [[nodiscard]] bool is_opened() const;

    /**
     * @brief Encode and write a single frame
     * @param frame BGR format image to encode
     * @return true if successful
     */
    bool write_frame(const cv::Mat& frame);

    /**
     * @brief Get the number of frames successfully written
     */
    [[nodiscard]] int get_written_frame_count() const;

    /**
     * @brief Set the audio source to copy audio from
     * @param sourceVideoPath Path to the video file containing the audio track
     */
    void set_audio_source(const std::string& sourceVideoPath);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace foundation::media::ffmpeg
