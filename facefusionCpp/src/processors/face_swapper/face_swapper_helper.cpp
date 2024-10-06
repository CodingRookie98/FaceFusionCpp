/**
 ******************************************************************************
 * @file           : face_swapper_helper.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-18
 ******************************************************************************
 */

#include "face_swapper_helper.h"
#include "model_manager.h"
#include "face_swapper_inswaper_128.h"

FaceRecognizers::FaceRecognizerType FaceSwapperHelper::getFaceRecognizerOfFaceSwapper(const FaceSwapperHelper::FaceSwapperModel &faceSwapper) {
    switch (faceSwapper) {
    case FaceSwapperModel::Inswapper_128:
    case FaceSwapperModel::Inswapper_128_fp16:
        return FaceRecognizers::FaceRecognizerType::Arc_w600k_r50;
    }
}

FaceSwapperBase *FaceSwapperHelper::createFaceSwapper(const FaceSwapperHelper::FaceSwapperModel &model,
                                                      std::shared_ptr<FaceMaskers> maskers,
                                                      const std::shared_ptr<Ort::Env> &env) {
    std::shared_ptr<Ort::Env> fsEnv = env;
    if (fsEnv == nullptr) {
        fsEnv = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceHelper");
    }

    using namespace Ffc;
    std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    auto mgrModel = static_cast<ModelManager::Model>(static_cast<int>(model));
    std::string modelPath = modelManager->getModelPath(mgrModel);

    if (model == Inswapper_128) {
        return new FaceSwapperInswaper128(fsEnv, maskers, modelPath, FaceSwapperInswaper128::Version::V128);
    }
    if (model == Inswapper_128_fp16) {
        return new FaceSwapperInswaper128(fsEnv, maskers, modelPath, FaceSwapperInswaper128::Version::V128_fp16);
    }
}
