/**
 ******************************************************************************
 * @file           : processor_pool.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-12-25
 ******************************************************************************
 */

module;
#include <optional>
#include <mutex>
#include <onnxruntime_cxx_api.h>

export module processor_hub:processor_pool;
export import processor_base;
export import face_swapper;
import face_masker_hub;
import inference_session;
export import face_enhancer;
export import expression_restorer;
export import frame_enhancer;
export import model_manager;

namespace ffc {

using namespace faceSwapper;
using namespace faceEnhancer;
using namespace faceMasker;
using namespace expressionRestore;
using namespace frameEnhancer;

export enum class ProcessorMajorType {
    FaceSwapper,
    FaceEnhancer,
    ExpressionRestorer,
    FrameEnhancer,
};

export enum class FaceSwapperType {
    InSwapper,
};

export enum class FaceEnhancerType {
    GFP_GAN,
    CodeFormer,
};

export enum class ExpressionRestorerType {
    LivePortrait,
};

export enum class FrameEnhancerType {
    Real_esr_gan,
    Real_hat_gan,
};

export enum class ProcessorMinorType {
    FaceSwapper_InSwapper,
    FaceEnhancer_GfpGan,
    FaceEnhancer_CodeFormer,
    ExpressionRestorer_LivePortrait,
    FrameEnhancer_RealEsrgan,
    FrameEnhancer_RealHatgan,
};

export std::optional<FaceSwapperType> GetFaceSwapperType(ProcessorMinorType type) {
    if (type == ProcessorMinorType::FaceSwapper_InSwapper) return FaceSwapperType::InSwapper;
    return std::nullopt;
}

export std::optional<FaceEnhancerType> GetFaceEnhancerType(ProcessorMinorType type) {
    if (type == ProcessorMinorType::FaceEnhancer_GfpGan) return FaceEnhancerType::GFP_GAN;
    if (type == ProcessorMinorType::FaceEnhancer_CodeFormer) return FaceEnhancerType::CodeFormer;
    return std::nullopt;
}

export std::optional<ExpressionRestorerType> GetExpressionRestorerType(ProcessorMinorType type) {
    if (type == ProcessorMinorType::ExpressionRestorer_LivePortrait) return ExpressionRestorerType::LivePortrait;
    return std::nullopt;
}

export std::optional<FrameEnhancerType> GetFrameEnhancerType(ProcessorMinorType type) {
    if (type == ProcessorMinorType::FrameEnhancer_RealEsrgan) return FrameEnhancerType::Real_esr_gan;
    if (type == ProcessorMinorType::FrameEnhancer_RealHatgan) return FrameEnhancerType::Real_hat_gan;
    return std::nullopt;
}

export class ProcessorPool {
public:
    explicit ProcessorPool(const InferenceSession::Options &_options);
    ~ProcessorPool();

    void removeProcessors(const ProcessorMajorType &_majorType);

    std::shared_ptr<FaceSwapperBase>
    getFaceSwapper(const FaceSwapperType &_faceSwapperType,
                   const ModelManager::Model &_model);

    std::shared_ptr<FaceEnhancerBase>
    getFaceEnhancer(const FaceEnhancerType &_faceEnhancerType,
                    const ModelManager::Model &_model);

    std::shared_ptr<ExpressionRestorerBase>
    getExpressionRestorer(const ExpressionRestorerType &_expressionRestorer);

    std::shared_ptr<FrameEnhancerBase>
    getFrameEnhancer(const FrameEnhancerType &_frameEnhancerType,
                     const ModelManager::Model &_model);

private:
    std::unordered_map<FaceSwapperType, std::pair<std::shared_ptr<FaceSwapperBase>, ModelManager::Model>> faceSwappers_;
    std::mutex mutex4FaceSwappers_;
    std::unordered_map<FaceEnhancerType, std::pair<std::shared_ptr<FaceEnhancerBase>, ModelManager::Model>> faceEnhancers_;
    std::mutex mutex4FaceEnhancers_;
    std::unordered_map<ExpressionRestorerType, std::shared_ptr<ExpressionRestorerBase>> expressionRestorers_;
    std::mutex mutex4ExpressionRestorers_;
    std::unordered_map<FrameEnhancerType, std::pair<std::shared_ptr<FrameEnhancerBase>, ModelManager::Model>> frameEnhancers_;
    std::mutex mutex4FrameEnhancers_;
    std::shared_ptr<FaceMaskerHub> faceMaskerHub_;
    std::shared_ptr<Ort::Env> env_;
    InferenceSession::Options is_options_;

    std::shared_ptr<LivePortrait> getLivePortrait();
};
} // namespace ffc
