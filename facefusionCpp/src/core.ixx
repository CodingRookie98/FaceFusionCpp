/**
 ******************************************************************************
 * @file           : core.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include <thread_pool/thread_pool.h>
#include <opencv2/opencv.hpp>

export module core;
import face_analyser;
export import :core_run_options;
import logger;
import processor_hub;

namespace ffc {
using namespace faceMasker;
using namespace std;

export class Core {
public:
    struct Options {
        // misc
        bool force_download{true};
        bool skip_download{false};
        Logger::LogLevel log_level{Logger::LogLevel::Trace};
        // execution
        unsigned short execution_thread_count{1};
        // memory
        enum class MemoryStrategy {
            Strict,
            Tolerant,
        };
        MemoryStrategy processor_memory_strategy{MemoryStrategy::Tolerant};
        InferenceSession::Options inference_session_options{};
    };

    explicit Core(const Core::Options &options);
    ~Core();

    [[nodiscard]] bool run(CoreRunOptions _coreRunOptions);

private:
    std::unique_ptr<dp::thread_pool<>> thread_pool_;
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<Ort::Env> m_env;
    std::shared_ptr<FaceAnalyser> m_faceAnalyser;
    ProcessorHub processor_hub_;
    Options coreOptions;

    bool processImages(CoreRunOptions _coreRunOptions);
    bool processVideos(const CoreRunOptions &_coreRunOptions, const bool &autoRemoveTarget = false);
    bool processVideo(CoreRunOptions _coreRunOptions);
    bool processVideoInSegments(CoreRunOptions _coreRunOptions);
    [[nodiscard]] bool processSourceAverageFace(CoreRunOptions _coreRunOptions) const;
    bool swapFace(CoreRunOptions _coreRunOptions);
    bool enhanceFace(CoreRunOptions _coreRunOptions);
    bool restoreExpression(CoreRunOptions _coreRunOptions);
    bool enhanceFrame(CoreRunOptions _coreRunOptions);
    [[nodiscard]] std::vector<Face> getTargetFaces(const CoreRunOptions &_coreRunOptions, const cv::Mat &targetFrame) const;
};

} // namespace ffc
