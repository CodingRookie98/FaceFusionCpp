/**
 ******************************************************************************
 * @file           : frame_enhancer_helper.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-31
 ******************************************************************************
 */

#include "frame_enhancer_helper.h"
#include "real_esr_gan.h"
#include "model_manager.h"

using namespace Ffc;

FrameEnhancerBase *FrameEnhancerHelper::CreateFrameEnhancer(FrameEnhancerHelper::Model model, const std::shared_ptr<Ort::Env> &env) {
    std::shared_ptr<Ort::Env> envPtr = env;
    if (env == nullptr) {
        envPtr = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FrameEnhancer");
    }

    FrameEnhancerBase *frame_enhancer = nullptr;
    if (model == Model::Real_esrgan_x2) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_esrgan_x2));
        frame_enhancer->setModelScale(2);
        frame_enhancer->setTileSize({256, 16, 8});
    }
    if (model == Model::Real_esrgan_x2_fp16) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_esrgan_x2_fp16));
        frame_enhancer->setModelScale(2);
        frame_enhancer->setTileSize({256, 16, 8});
    }
    if (model == Model::Real_esrgan_x4) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_esrgan_x4));
        frame_enhancer->setModelScale(4);
        frame_enhancer->setTileSize({256, 16, 8});
    }
    if (model == Model::Real_esrgan_x4_fp16) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_esrgan_x4_fp16));
        frame_enhancer->setModelScale(4);
        frame_enhancer->setTileSize({256, 16, 8});
    }
    if (model == Model::Real_esrgan_x8) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_esrgan_x8));
        frame_enhancer->setModelScale(8);
        frame_enhancer->setTileSize({256, 16, 8});
    }
    if (model == Model::Real_esrgan_x8_fp16) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_esrgan_x8_fp16));
        frame_enhancer->setModelScale(8);
        frame_enhancer->setTileSize({256, 16, 8});
    }
    if (model == Model::Real_hatgan_x4) {
        frame_enhancer = new RealEsrGan(envPtr, ModelManager::getInstance()->getModelPath(ModelManager::Real_hatgan_x4));
        frame_enhancer->setModelScale(4);
        frame_enhancer->setTileSize({256, 16, 8});
    }

    return frame_enhancer;
}
