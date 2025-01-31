/**
 ******************************************************************************
 * @file           : processor_hub.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 25-1-7
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

export module processor_hub;
export import :processor_pool;
export import face_swapper;
export import face_enhancer;
export import expression_restorer;
import inference_session;

namespace ffc {

export struct FaceSwapperInput {
    std::optional<InSwapperInput> in_swapper_input;
};

export struct FaceEnhancerInput {
    std::optional<CodeFormerInput> code_former_input;
    std::optional<GFP_GAN_Input> gfp_gan_input;
};

export struct ExpressionRestorerInput {
    std::optional<LivePortraitInput> live_portrait_input;
};

export struct FrameEnhancerInput {
    std::optional<RealEsrGanInput> real_esr_gan_input;
    std::optional<RealHatGanInput> real_hat_gan_input;
};

export class ProcessorHub {
public:
    explicit ProcessorHub(const InferenceSession::Options &_options);
    ~ProcessorHub() = default;

    ProcessorPool &getProcessorPool() {
        return processorPool_;
    }

    cv::Mat swapFace(const FaceSwapperType &_faceSwapperType,
                     const ModelManager::Model &_model,
                     const FaceSwapperInput &_faceSwapperInput);

    cv::Mat enhanceFace(const FaceEnhancerType &_faceEnhancerType,
                        const ModelManager::Model &_model,
                        const FaceEnhancerInput &_faceEnhancerInput);

    cv::Mat restoreExpression(const ExpressionRestorerType &_type,
                              const ExpressionRestorerInput &input);

    cv::Mat enhanceFrame(const FrameEnhancerType &_frameEnhancerType,
                         const ModelManager::Model &_model,
                         const FrameEnhancerInput &_input);

private:
    ProcessorPool processorPool_;
};
} // namespace ffc