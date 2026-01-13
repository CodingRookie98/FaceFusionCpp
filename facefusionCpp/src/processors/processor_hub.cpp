/**
 ******************************************************************************
 * @file           : processor_hub.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 25-1-7
 ******************************************************************************
 */

module;
#include <memory>
#include <stdexcept>

module processor_hub;
import :processor_pool;

namespace ffc {

ProcessorHub::ProcessorHub(const InferenceSession::Options& _options) : processorPool_(_options) {}

cv::Mat ProcessorHub::swapFace(const FaceSwapperType& _faceSwapperType,
                               const model_manager::Model& _model,
                               const FaceSwapperInput& _faceSwapperInput) {
    if (_faceSwapperType == FaceSwapperType::InSwapper) {
        const auto ptr = std::dynamic_pointer_cast<InSwapper>(
            processorPool_.get_face_swapper(_faceSwapperType, _model));
        if (!_faceSwapperInput.in_swapper_input) {
            throw std::invalid_argument("Invalid input for InSwapper");
        }
        return ptr->swapFace(*_faceSwapperInput.in_swapper_input);
    }
    return {};
}

cv::Mat ProcessorHub::enhanceFace(const FaceEnhancerType& _faceEnhancerType,
                                  const model_manager::Model& _model,
                                  const FaceEnhancerInput& _faceEnhancerInput) {
    if (_faceEnhancerType == FaceEnhancerType::CodeFormer) {
        const auto ptr = std::dynamic_pointer_cast<CodeFormer>(
            processorPool_.get_face_enhancer(_faceEnhancerType, _model));
        if (!_faceEnhancerInput.code_former_input) {
            throw std::invalid_argument("Invalid input for CodeFormer");
        }
        return ptr->enhanceFace(*_faceEnhancerInput.code_former_input);
    }

    if (_faceEnhancerType == FaceEnhancerType::GFP_GAN) {
        const auto ptr = std::dynamic_pointer_cast<GFP_GAN>(
            processorPool_.get_face_enhancer(_faceEnhancerType, _model));
        if (!_faceEnhancerInput.gfp_gan_input) {
            throw std::invalid_argument("Invalid input for GFP_GAN");
        }
        return ptr->enhanceFace(*_faceEnhancerInput.gfp_gan_input);
    }

    return {};
}

cv::Mat ProcessorHub::restoreExpression(const ExpressionRestorerType& _type,
                                        const ExpressionRestorerInput& input) {
    if (_type == ExpressionRestorerType::LivePortrait) {
        const auto ptr = std::dynamic_pointer_cast<LivePortrait>(
            processorPool_.get_expression_restorer(ExpressionRestorerType::LivePortrait));
        if (!input.live_portrait_input) {
            throw std::invalid_argument("Invalid input for LivePortrait");
        }
        return ptr->restoreExpression(*input.live_portrait_input);
    }
    return {};
}

cv::Mat ProcessorHub::enhanceFrame(const FrameEnhancerType& _frameEnhancerType,
                                   const model_manager::Model& _model,
                                   const FrameEnhancerInput& _input) {
    if (_frameEnhancerType == FrameEnhancerType::Real_esr_gan) {
        const auto ptr = std::dynamic_pointer_cast<RealEsrGan>(
            processorPool_.get_frame_enhancer(_frameEnhancerType, _model));
        if (!_input.real_esr_gan_input) {
            throw std::invalid_argument("Invalid input for RealEsrGAN");
        }
        return ptr->enhanceFrame(*_input.real_esr_gan_input);
    }

    if (_frameEnhancerType == FrameEnhancerType::Real_hat_gan) {
        const auto ptr = std::dynamic_pointer_cast<RealHatGan>(
            processorPool_.get_frame_enhancer(_frameEnhancerType, _model));
        if (!_input.real_hat_gan_input) {
            throw std::invalid_argument("Invalid input for RealHatGAN");
        }
        return ptr->enhanceFrame(*_input.real_hat_gan_input);
    }
    return {};
}

} // namespace ffc