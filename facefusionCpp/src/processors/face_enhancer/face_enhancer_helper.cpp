/**
 ******************************************************************************
 * @file           : face_enhancer_helper.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-21
 ******************************************************************************
 */

#include "face_enhancer_helper.h"
#include "face_enhancer_gfpgan.h"
#include "code_former.h"

FaceEnhancerBase *FaceEnhancerHelper::createFaceEnhancer(const FaceEnhancerHelper::Model &model,
                                                         const std::shared_ptr<FaceMaskers>& maskers,
                                                         const std::shared_ptr<Ort::Env> &env) {
    std::shared_ptr<Ort::Env> envPtr = env;
    if (envPtr == nullptr) {
        envPtr = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceEnhancer");
    }

    using namespace Ffc;
    std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    auto mgrModel = static_cast<ModelManager::Model>(static_cast<int>(model));
    std::string modelPath = modelManager->getModelPath(mgrModel);

    if (model == Gfpgan_12 || model == Gfpgan_13 || model == Gfpgan_14) {
        return new FaceEnhancerGfpgan(envPtr, maskers, modelPath);
    } else if (model == _CodeFormer) {
        return new CodeFormer(envPtr, maskers, modelPath);
    }
}
