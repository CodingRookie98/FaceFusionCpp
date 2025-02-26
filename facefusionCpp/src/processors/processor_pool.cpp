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

ProcessorPool::ProcessorPool(const InferenceSession::Options &_options) {
    env_ = std::make_shared<Ort::Env>(Ort::Env(ORT_LOGGING_LEVEL_ERROR, "faceFusionCpp"));
    is_options_ = _options;
}

ProcessorPool::~ProcessorPool() {
    if (faceMaskerHub_) {
        faceMaskerHub_.reset();
    }
    {
        std::lock_guard lock(mutex4FaceSwappers_);
        faceSwappers_.clear();
    }
    {
        std::lock_guard lock(mutex4FaceEnhancers_);
        faceEnhancers_.clear();
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

void ProcessorPool::removeProcessors(const ProcessorMajorType &_majorType) {
    if (_majorType == ProcessorMajorType::FaceSwapper) {
        std::lock_guard lock(mutex4FaceSwappers_);
        faceSwappers_.clear();
    }
    if (_majorType == ProcessorMajorType::FaceEnhancer) {
        std::lock_guard lock(mutex4FaceEnhancers_);
        faceEnhancers_.clear();
    }
    if (_majorType == ProcessorMajorType::ExpressionRestorer) {
        std::lock_guard lock(mutex4ExpressionRestorers_);
        expressionRestorers_.clear();
    }
    if (_majorType == ProcessorMajorType::FrameEnhancer) {
        std::lock_guard lock(mutex4FrameEnhancers_);
        frameEnhancers_.clear();
    }
}

std::shared_ptr<FaceSwapperBase>
ProcessorPool::getFaceSwapper(const FaceSwapperType &_faceSwapperType, const ModelManager::Model &_model) {
    std::lock_guard lock(mutex4FaceSwappers_);
    if (faceSwappers_.contains(_faceSwapperType)) {
        return faceSwappers_[_faceSwapperType];
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }

    std::shared_ptr<FaceSwapperBase> ptr = nullptr;
    if (_faceSwapperType == FaceSwapperType::InSwapper) {
        if (_model != ModelManager::Model::Inswapper_128 && _model != ModelManager::Model::Inswapper_128_fp16) {
            throw std::invalid_argument("model is not supported for inswapper!");
        }
        const auto inSwapper = std::make_shared<InSwapper>(env_);
        inSwapper->LoadModel(ModelManager::getInstance()->getModelPath(_model), is_options_);
        if (!inSwapper->hasFaceMaskerHub()) {
            inSwapper->setFaceMaskerHub(faceMaskerHub_);
        }
        ptr = inSwapper;
    }

    if (ptr) {
        faceSwappers_.insert({_faceSwapperType, ptr});
    }
    return ptr;
}

std::shared_ptr<FaceEnhancerBase>
ProcessorPool::getFaceEnhancer(const FaceEnhancerType &_faceEnhancerType,
                               const ModelManager::Model &_model) {
    std::lock_guard lock(mutex4FaceEnhancers_);
    if (faceEnhancers_.contains(_faceEnhancerType)) {
        return faceEnhancers_[_faceEnhancerType];
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }

    std::shared_ptr<FaceEnhancerBase> ptr = nullptr;
    if (_faceEnhancerType == FaceEnhancerType::CodeFormer) {
        if (_model != ModelManager::Model::Codeformer) {
            throw std::invalid_argument("model is not supported for codeformer!");
        }
        const auto codeFormer = std::make_shared<CodeFormer>(env_);
        codeFormer->LoadModel(ModelManager::getInstance()->getModelPath(_model), is_options_);
        if (!codeFormer->hasFaceMaskerHub()) {
            codeFormer->setFaceMaskerHub(faceMaskerHub_);
        }
        ptr = codeFormer;
    }

    if (_faceEnhancerType == FaceEnhancerType::GFP_GAN) {
        if (_model != ModelManager::Model::Gfpgan_12 && _model != ModelManager::Model::Gfpgan_13 && _model != ModelManager::Model::Gfpgan_14) {
            throw std::invalid_argument("model is not supported for gfpgan!");
        }
        const auto gfpgan = std::make_shared<GFP_GAN>(env_);
        gfpgan->LoadModel(ModelManager::getInstance()->getModelPath(_model), is_options_);
        if (!gfpgan->hasFaceMaskerHub()) {
            gfpgan->setFaceMaskerHub(faceMaskerHub_);
        }
        ptr = gfpgan;
    }

    if (ptr) {
        faceEnhancers_.insert({_faceEnhancerType, ptr});
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
        ptr->loadModel(ModelManager::getInstance()->getModelPath(ModelManager::Model::Feature_extractor),
                       ModelManager::getInstance()->getModelPath(ModelManager::Model::Motion_extractor),
                       ModelManager::getInstance()->getModelPath(ModelManager::Model::Generator),
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
ProcessorPool::getExpressionRestorer(const ExpressionRestorerType &_expressionRestorer) {
    std::lock_guard lock(mutex4ExpressionRestorers_);
    if (expressionRestorers_.contains(_expressionRestorer)) {
        return expressionRestorers_[_expressionRestorer];
    }
    if (faceMaskerHub_ == nullptr) {
        faceMaskerHub_ = std::make_shared<FaceMaskerHub>(env_, is_options_);
    }

    std::shared_ptr<ExpressionRestorerBase> ptr = nullptr;
    if (_expressionRestorer == ExpressionRestorerType::LivePortrait) {
        ptr = getLivePortrait();
    }

    if (ptr) {
        expressionRestorers_.insert({_expressionRestorer, ptr});
    }
    return ptr;
}

std::shared_ptr<FrameEnhancerBase>
ProcessorPool::getFrameEnhancer(const FrameEnhancerType &_frameEnhancerType,
                                const ModelManager::Model &_model) {
    std::lock_guard lock(mutex4FrameEnhancers_);
    if (frameEnhancers_.contains(_frameEnhancerType)) {
        return frameEnhancers_[_frameEnhancerType];
    }

    std::shared_ptr<FrameEnhancerBase> ptr = nullptr;
    if (_frameEnhancerType == FrameEnhancerType::Real_esr_gan) {
        if (_model != ModelManager::Model::Real_esrgan_x2
            && _model != ModelManager::Model::Real_esrgan_x2_fp16
            && _model != ModelManager::Model::Real_esrgan_x4
            && _model != ModelManager::Model::Real_esrgan_x4_fp16
            && _model != ModelManager::Model::Real_esrgan_x8
            && _model != ModelManager::Model::Real_esrgan_x8_fp16) {
            throw std::invalid_argument("model is not supported for real_esrgan!");
        }

        const auto realEsrGan = std::make_shared<RealEsrGan>(env_);
        realEsrGan->LoadModel(ModelManager::getInstance()->getModelPath(_model), is_options_);
        if (_model == ModelManager::Model::Real_esrgan_x2
            || _model == ModelManager::Model::Real_esrgan_x2_fp16) {
            realEsrGan->setModelScale(2);
            realEsrGan->setTileSize({256, 16, 8});
        }
        if (_model == ModelManager::Model::Real_esrgan_x4
            || _model == ModelManager::Model::Real_esrgan_x4_fp16) {
            realEsrGan->setModelScale(4);
            realEsrGan->setTileSize({256, 16, 8});
        }
        if (_model == ModelManager::Model::Real_esrgan_x8
            || _model == ModelManager::Model::Real_esrgan_x8_fp16) {
            realEsrGan->setModelScale(8);
            realEsrGan->setTileSize({256, 16, 8});
        }

        ptr = realEsrGan;
    }

    if (_frameEnhancerType == FrameEnhancerType::Real_hat_gan) {
        if (_model != ModelManager::Model::Real_hatgan_x4) {
            throw std::invalid_argument("model is not supported for real_hat_gan!");
        }
        const auto realHatGAN = std::make_shared<RealHatGan>(env_);
        realHatGAN->LoadModel(ModelManager::getInstance()->getModelPath(_model), is_options_);
        if (_model == ModelManager::Model::Real_hatgan_x4) {
            realHatGAN->setModelScale(4);
            realHatGAN->setTileSize({256, 16, 8});
        }
        ptr = realHatGAN;
    }

    if (ptr) {
        frameEnhancers_.insert({_frameEnhancerType, ptr});
    }
    return ptr;
}

} // namespace ffc