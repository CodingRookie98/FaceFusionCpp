/**
 ******************************************************************************
 * @file           : ffmpeg_runner.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-8-6
 ******************************************************************************
 */
module;
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

export module ffmpeg_runner;

namespace ffc::media {
export enum Audio_Codec {
    Codec_AAC,
    Codec_MP3,
    Codec_OPUS,
    Codec_VORBIS,
    Codec_UNKNOWN
};

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

export std::vector<std::string> childProcess(const std::string& command);
export bool isVideo(const std::string& videoPath);
export bool isAudio(const std::string& videoPath);
export void extractFrames(const std::string& videoPath, const std::string& outputImagePattern);
export void extractAudios(const std::string& videoPath, const std::string& outputDir, const Audio_Codec& audioCodec = Codec_AAC);
export bool addAudiosToVideo(const std::string& videoPath, const std::vector<std::string>& audioPaths,
                             const std::string& outputVideoPath);

// no audio output
export bool cutVideoIntoSegments(const std::string& videoPath, const std::string& outputPath,
                                 const unsigned int& segmentDuration, const std::string& outputPattern);

export bool concatVideoSegments(const std::vector<std::string>& videoSegmentsPaths,
                                const std::string& outputVideoPath, const VideoPrams& videoPrams);

export std::unordered_map<unsigned int, std::string> getAudioStreamsIndexAndCodec(const std::string& videoPath);

export std::unordered_set<std::string> filterVideoPaths(const std::unordered_set<std::string>& filePaths);
export std::unordered_set<std::string> filterAudioPaths(const std::unordered_set<std::string>& filePaths);

export bool imagesToVideo(const std::string& inputImagePattern, const std::string& outputVideoPath,
                          const VideoPrams& videoPrams);
export Audio_Codec getAudioCodec(const std::string& codec);

std::string map_NVENC_preset(const std::string& preset);
std::string map_amf_preset(const std::string& preset);
std::string getCompressionAndPresetCmd(const unsigned int& quality, const std::string& preset, const std::string& codec);

} // namespace ffc::media
