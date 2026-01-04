/**
 ******************************************************************************
 * @file           : processor_pool.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-12-25
 ******************************************************************************
 */

module;
#include <mutex>
#include <onnxruntime_cxx_api.h>

module processor_hub;
import :processor_pool;

using namespace ffc::faceSwapper;

namespace ffc {

using namespace ffc::ai;
using namespace ffc::ai::model_manager;

ProcessorPool::ProcessorPool(const InferenceSession::Options& options) {
    env_ = std::make_shared<Ort::Env>(Ort::Env(ORT_LOGGING_LEVEL_ERROR, "faceFusionCpp"));
    is_options_ = options;
}

ProcessorPool::~ProcessorPool() {
    if (faceMaskerHub_) {
        faceMaskerHub_.reset();
    }
    {
        std::lock_guard lock(m_mutex_face_swappers);
        m_face_swappers.clear();
    }
    {
        std::lock_guard lock(mutex4FaceEnhancers_);
        m_face_enhancers.clear();
    }
    {
        std::lock_guard lock(mutex4ExpressionRestorers_);
        expressionRestorers_.clear();
    }
    {
        std::lock_guard lock(mutex4FrameEnhancers_);
        frameEnhancers_.clear();
    }
}

void ProcessorPool::remove_processors(const ProcessorMajorType& major_type) {
    if (major_type == ProcessorMajorType::FaceSwapper) {
        std::lock_guard lock(m_mutex_face_swappers);
        m_face_swappers.clear();
    }
    if (major_type == ProcessorMajorType::FaceEnhancer) {
        std::lock_guard lock(mutex4FaceEnhancers_);
        m_face_enhancers.clear();
    }
    if (major_type == ProcessorMajorType::ExpressionRestorer) {
        std::lock_guard lock(mutex4ExpressionRestorers_);
        expressionRestorers_.clear();
    }
    if (major_type == ProcessorMajorType::FrameEnhancer) {
        std::lock_guard lock(mutex4FrameEnhancers_);
        frameEnhancers_.clear();
    }
}

std::shared_ptr<FaceSwapperBase>
ProcessorPool::get_face_swapper(const FaceSwapperType& face_swapper_type, const model_manager::Model& model) {
    std::lock_guard lock(m_mutex_face_swappers);
    if (m_face_swappers.contains(face_swapper_type)) {
        if (m_face_swappers[face_swapper_type].second == model) {
            return m_face_swappers[face_swapper_type].first;
        }
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }

    std::shared_ptr<FaceSwapperBase> ptr = nullptr;
    if (face_swapper_type == FaceSwapperType::InSwapper) {
        if (model != model_manager::Model::Inswapper_128 && model != model_manager::Model::Inswapper_128_fp16) {
            throw std::invalid_argument("model is not supported for inswapper!");
        }
        const auto inSwapper = std::make_shared<InSwapper>(env_);
        inSwapper->load_model(ModelManager::get_instance()->get_model_path(model), is_options_);
        if (!inSwapper->has_face_masker_hub()) {
            inSwapper->set_face_masker_hub(faceMaskerHub_);
        }
        ptr = inSwapper;
    }

    if (ptr) {
        m_face_swappers[face_swapper_type] = {ptr, model};
    }
    return ptr;
}

std::shared_ptr<FaceEnhancerBase>
ProcessorPool::get_face_enhancer(const FaceEnhancerType& face_enhancer_type,
                                 const model_manager::Model& model) {
    std::lock_guard lock(mutex4FaceEnhancers_);
    if (m_face_enhancers.contains(face_enhancer_type)) {
        if (m_face_enhancers[face_enhancer_type].second == model) {
            return m_face_enhancers[face_enhancer_type].first;
        }
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }

    std::shared_ptr<FaceEnhancerBase> ptr = nullptr;
    if (face_enhancer_type == FaceEnhancerType::CodeFormer) {
        if (model != model_manager::Model::Codeformer) {
            throw std::invalid_argument("model is not supported for codeformer!");
        }
        const auto codeFormer = std::make_shared<CodeFormer>(env_);
        codeFormer->load_model(ModelManager::get_instance()->get_model_path(model), is_options_);
        if (!codeFormer->hasFaceMaskerHub()) {
            codeFormer->setFaceMaskerHub(faceMaskerHub_);
        }
        ptr = codeFormer;
    }

    if (face_enhancer_type == FaceEnhancerType::GFP_GAN) {
        if (model != model_manager::Model::Gfpgan_12 && model != model_manager::Model::Gfpgan_13 && model != model_manager::Model::Gfpgan_14) {
            throw std::invalid_argument("model is not supported for gfpgan!");
        }
        const auto gfpgan = std::make_shared<GFP_GAN>(env_);
        gfpgan->load_model(ModelManager::get_instance()->get_model_path(model), is_options_);
        if (!gfpgan->hasFaceMaskerHub()) {
            gfpgan->setFaceMaskerHub(faceMaskerHub_);
        }
        ptr = gfpgan;
    }

    if (ptr) {
        m_face_enhancers[face_enhancer_type] = {ptr, model};
    }
    return ptr;
}

std::shared_ptr<LivePortrait> ProcessorPool::getLivePortrait() {
    // 由 getExpressionRestorer 调用并lock
    // std::lock_guard lock(mutex4ExpressionRestorers_);
    if (expressionRestorers_.contains(ExpressionRestorerType::LivePortrait)) {
        return std::dynamic_pointer_cast<LivePortrait>(expressionRestorers_[ExpressionRestorerType::LivePortrait]);
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }
    const auto ptr = std::make_shared<LivePortrait>(env_);
    if (!ptr->isModelLoaded()) {
        ptr->loadModel(ModelManager::get_instance()->get_model_path(model_manager::Model::Feature_extractor),
                       ModelManager::get_instance()->get_model_path(model_manager::Model::Motion_extractor),
                       ModelManager::get_instance()->get_model_path(model_manager::Model::Generator),
                       is_options_);
    }
    if (!ptr->hasFaceMaskers()) {
        ptr->setFaceMaskers(faceMaskerHub_);
    }
    if (ptr) {
        expressionRestorers_.insert({ExpressionRestorerType::LivePortrait, ptr});
    }
    return ptr;
}

std::shared_ptr<ExpressionRestorerBase>
ProcessorPool::get_expression_restorer(const ExpressionRestorerType& expression_restorer) {
    std::lock_guard lock(mutex4ExpressionRestorers_);
    if (expressionRestorers_.contains(expression_restorer)) {
        return expressionRestorers_[expression_restorer];
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }

    std::shared_ptr<ExpressionRestorerBase> ptr = nullptr;
    if (expression_restorer == ExpressionRestorerType::LivePortrait) {
        ptr = getLivePortrait();
    }

    if (ptr) {
        expressionRestorers_.insert({expression_restorer, ptr});
    }
    return ptr;
}

std::shared_ptr<FrameEnhancerBase>
ProcessorPool::get_frame_enhancer(const FrameEnhancerType& frame_enhancer_type,
                                  const model_manager::Model& model) {
    std::lock_guard lock(mutex4FrameEnhancers_);
    if (frameEnhancers_.contains(frame_enhancer_type)) {
        if (frameEnhancers_[frame_enhancer_type].second == model) {
            return frameEnhancers_[frame_enhancer_type].first;
        }
    }

    std::shared_ptr<FrameEnhancerBase> ptr = nullptr;
    if (frame_enhancer_type == FrameEnhancerType::Real_esr_gan) {
        if (model != model_manager::Model::Real_esrgan_x2
            && model != model_manager::Model::Real_esrgan_x2_fp16
            && model != model_manager::Model::Real_esrgan_x4
            && model != model_manager::Model::Real_esrgan_x4_fp16
            && model != model_manager::Model::Real_esrgan_x8
            && model != model_manager::Model::Real_esrgan_x8_fp16) {
            throw std::invalid_argument("model is not supported for real_esrgan!");
        }

        const auto realEsrGan = std::make_shared<RealEsrGan>(env_);
        realEsrGan->load_model(ModelManager::get_instance()->get_model_path(model), is_options_);
        if (model == model_manager::Model::Real_esrgan_x2
            || model == model_manager::Model::Real_esrgan_x2_fp16) {
            realEsrGan->setModelScale(2);
            realEsrGan->setTileSize({256, 16, 8});
        }
        if (model == model_manager::Model::Real_esrgan_x4
            || model == model_manager::Model::Real_esrgan_x4_fp16) {
            realEsrGan->setModelScale(4);
            realEsrGan->setTileSize({256, 16, 8});
        }
        if (model == model_manager::Model::Real_esrgan_x8
            || model == model_manager::Model::Real_esrgan_x8_fp16) {
            realEsrGan->setModelScale(8);
            realEsrGan->setTileSize({256, 16, 8});
        }

        ptr = realEsrGan;
    }

    if (frame_enhancer_type == FrameEnhancerType::Real_hat_gan) {
        if (model != model_manager::Model::Real_hatgan_x4) {
            throw std::invalid_argument("model is not supported for real_hat_gan!");
        }
        const auto realHatGAN = std::make_shared<RealHatGan>(env_);
        realHatGAN->load_model(ModelManager::get_instance()->get_model_path(model), is_options_);
        if (model == model_manager::Model::Real_hatgan_x4) {
            realHatGAN->setModelScale(4);
            realHatGAN->setTileSize({256, 16, 8});
        }
        ptr = realHatGAN;
    }

    if (ptr) {
        frameEnhancers_[frame_enhancer_type] = {ptr, model};
    }
    return ptr;
}

} // namespace ffc