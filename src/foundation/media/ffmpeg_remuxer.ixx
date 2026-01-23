module;
#include <string>
#include <vector>

export module foundation.media.ffmpeg:remuxer;

namespace foundation::media::ffmpeg {

export class Remuxer {
public:
    /**
     * @brief Merges video from video_path and audio from audio_path into output_path
     * @param video_path Path to the video file (source of video stream)
     * @param audio_path Path to the audio file (source of audio stream)
     * @param output_path Path to the output file
     * @return true if successful
     */
    static bool merge_av(const std::string& video_path, const std::string& audio_path,
                         const std::string& output_path);
};

} // namespace foundation::media::ffmpeg
