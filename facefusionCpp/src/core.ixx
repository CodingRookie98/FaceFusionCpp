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
#include <opencv2/opencv.hpp>

export module core;
import face_analyser;
export import :core_task;
import logger;
import processor_hub;
import core_options;
import progress_observer;

namespace ffc {
using namespace faceMasker;
using namespace std;

export class Core {
public:
    explicit Core(const CoreOptions& options);
    ~Core();

    [[nodiscard]] bool Run(CoreTask core_task);
    void RegisterObserver(std::shared_ptr<IProgressObserver> observer) {
        observer_ = observer;
    }

private:
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<Ort::Env> env_;
    std::shared_ptr<FaceAnalyser> face_analyser_;
    ProcessorHub processor_hub_;
    CoreOptions core_options_;
    std::shared_ptr<IProgressObserver> observer_{nullptr};

    bool ProcessImages(CoreTask core_task);
    bool ProcessVideos(const CoreTask& core_task, const bool& autoRemoveTarget = false);
    bool ProcessVideo(CoreTask core_task);
    bool ProcessVideoInSegments(CoreTask core_task);
    bool SwapFace(const FaceSwapperInput& face_swapper_input, const std::string& output_path,
                  const FaceSwapperType& type, const ModelManager::Model& model);
    bool EnhanceFace(const FaceEnhancerInput& face_enhancer_input, const std::string& output_path,
                     const FaceEnhancerType& type, const ModelManager::Model& model);
    bool RestoreExpression(const ExpressionRestorerInput& expression_restorer_input,
                           const std::string& output_path,
                           const ExpressionRestorerType& type);
    bool EnhanceFrame(const FrameEnhancerInput& frame_enhancer_input,
                      const std::string& output_path,
                      const FrameEnhancerType& type, const ModelManager::Model& model);
};

} // namespace ffc
