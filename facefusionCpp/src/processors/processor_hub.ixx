module;
#include <opencv2/opencv.hpp>
#include <memory>

export module processor_hub;
export import :processor_pool;
export import domain.face.enhancer;
export import expression_restorer;
import inference_session;

namespace ffc {

using namespace ai;
using namespace domain::face::enhancer;

export struct FaceSwapperInput {
    std::unique_ptr<InSwapperInput> in_swapper_input{nullptr};
};

export struct FaceEnhancerInput {
    // Both enhancers now use the same input structure
    std::unique_ptr<EnhanceInput> enhance_input{nullptr};
};

export struct ExpressionRestorerInput {
    std::unique_ptr<LivePortraitInput> live_portrait_input;
};

export struct FrameEnhancerInput {
    std::unique_ptr<RealEsrGanInput> real_esr_gan_input;
    std::unique_ptr<RealHatGanInput> real_hat_gan_input;
};

export class ProcessorHub {
public:
    explicit ProcessorHub(const InferenceSession::Options& _options);
    ~ProcessorHub() = default;

    ProcessorPool& getProcessorPool() { return processorPool_; }

    cv::Mat swapFace(const FaceSwapperType& _faceSwapperType, const model_manager::Model& _model,
                     const FaceSwapperInput& _faceSwapperInput);

    cv::Mat enhanceFace(const FaceEnhancerType& _faceEnhancerType,
                        const model_manager::Model& _model,
                        const FaceEnhancerInput& _faceEnhancerInput);

    cv::Mat restoreExpression(const ExpressionRestorerType& _type,
                              const ExpressionRestorerInput& input);

    cv::Mat enhanceFrame(const FrameEnhancerType& _frameEnhancerType,
                         const model_manager::Model& _model, const FrameEnhancerInput& _input);

private:
    ProcessorPool processorPool_;
};
} // namespace ffc
