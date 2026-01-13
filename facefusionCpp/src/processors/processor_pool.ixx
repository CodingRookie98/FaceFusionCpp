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
using namespace face_masker;
using namespace expressionRestore;
using namespace frame_enhancer;

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

export std::optional<FaceSwapperType> get_face_swapper_type(ProcessorMinorType type) {
    if (type == ProcessorMinorType::FaceSwapper_InSwapper) return FaceSwapperType::InSwapper;
    return std::nullopt;
}

export std::optional<FaceEnhancerType> get_face_enhancer_type(ProcessorMinorType type) {
    if (type == ProcessorMinorType::FaceEnhancer_GfpGan) return FaceEnhancerType::GFP_GAN;
    if (type == ProcessorMinorType::FaceEnhancer_CodeFormer) return FaceEnhancerType::CodeFormer;
    return std::nullopt;
}

export std::optional<ExpressionRestorerType> get_expression_restorer_type(ProcessorMinorType type) {
    if (type == ProcessorMinorType::ExpressionRestorer_LivePortrait)
        return ExpressionRestorerType::LivePortrait;
    return std::nullopt;
}

export std::optional<FrameEnhancerType> get_frame_enhancer_type(ProcessorMinorType type) {
    if (type == ProcessorMinorType::FrameEnhancer_RealEsrgan)
        return FrameEnhancerType::Real_esr_gan;
    if (type == ProcessorMinorType::FrameEnhancer_RealHatgan)
        return FrameEnhancerType::Real_hat_gan;
    return std::nullopt;
}

export class ProcessorPool {
public:
    explicit ProcessorPool(const ai::InferenceSession::Options& options);
    ~ProcessorPool();

    void remove_processors(const ProcessorMajorType& major_type);

    std::shared_ptr<FaceSwapperBase> get_face_swapper(const FaceSwapperType& face_swapper_type,
                                                      const ai::model_manager::Model& model);

    std::shared_ptr<FaceEnhancerBase> get_face_enhancer(const FaceEnhancerType& face_enhancer_type,
                                                        const ai::model_manager::Model& model);

    std::shared_ptr<ExpressionRestorerBase> get_expression_restorer(
        const ExpressionRestorerType& expression_restorer);

    std::shared_ptr<FrameEnhancerBase> get_frame_enhancer(
        const FrameEnhancerType& frame_enhancer_type, const ai::model_manager::Model& model);

private:
    std::unordered_map<FaceSwapperType,
                       std::pair<std::shared_ptr<FaceSwapperBase>, ai::model_manager::Model>>
        m_face_swappers;
    std::mutex m_mutex_face_swappers;
    std::unordered_map<FaceEnhancerType,
                       std::pair<std::shared_ptr<FaceEnhancerBase>, ai::model_manager::Model>>
        m_face_enhancers;
    std::mutex mutex4FaceEnhancers_;
    std::unordered_map<ExpressionRestorerType, std::shared_ptr<ExpressionRestorerBase>>
        expressionRestorers_;
    std::mutex mutex4ExpressionRestorers_;
    std::unordered_map<FrameEnhancerType,
                       std::pair<std::shared_ptr<FrameEnhancerBase>, ai::model_manager::Model>>
        frameEnhancers_;
    std::mutex mutex4FrameEnhancers_;
    std::shared_ptr<FaceMaskerHub> faceMaskerHub_;
    std::shared_ptr<Ort::Env> env_;
    ai::InferenceSession::Options is_options_;

    std::shared_ptr<LivePortrait> getLivePortrait();
};
} // namespace ffc
