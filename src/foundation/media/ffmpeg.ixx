/**
 ******************************************************************************
 * @file           : ffmpeg.ixx
 * @brief          : FFmpeg runner module interface
 ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

export module foundation.media.ffmpeg;

export namespace foundation::media::ffmpeg {
    enum Audio_Codec {
        Codec_AAC,
        Codec_MP3,
        Codec_OPUS,
        Codec_VORBIS,
        Codec_UNKNOWN
    };

    class VideoPrams {
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
    export void extract_audios(const std::string& videoPath, const std::string& outputDir, const Audio_Codec& audioCodec = Codec_AAC);
    export bool add_audios_to_video(const std::string& videoPath, const std::vector<std::string>& audioPaths,
                                    const std::string& outputVideoPath);

    // no audio output
    export bool cut_video_into_segments(const std::string& videoPath, const std::string& outputPath,
                                        const unsigned int& segmentDuration, const std::string& outputPattern);

    export bool concat_video_segments(const std::vector<std::string>& videoSegmentsPaths,
                                      const std::string& outputVideoPath, const VideoPrams& videoPrams);

    export std::unordered_map<unsigned int, std::string> get_audio_streams_index_and_codec(const std::string& videoPath);

    export std::unordered_set<std::string> filter_video_paths(const std::unordered_set<std::string>& filePaths);
    export std::unordered_set<std::string> filter_audio_paths(const std::unordered_set<std::string>& filePaths);

    export bool images_to_video(const std::string& inputImagePattern, const std::string& outputVideoPath,
                                const VideoPrams& videoPrams);
    export Audio_Codec get_audio_codec(const std::string& codec);

} // namespace foundation::media::ffmpeg
