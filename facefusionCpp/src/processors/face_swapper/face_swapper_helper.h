/**
 ******************************************************************************
 * @file           : face_swapper_helper.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-18
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_SWAPPER_FACE_SWAPPER_HELPER_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_SWAPPER_FACE_SWAPPER_HELPER_H_

#include "face_recognizers.h"
#include "face_maskers.h"
#include "face_swapper_base.h"
#include "model_manager.h"

class FaceSwapperHelper {
public:
    enum FaceSwapperModel {
        Inswapper_128 = Ffc::ModelManager::Model::Inswapper_128,
        Inswapper_128_fp16 = Ffc::ModelManager::Model::Inswapper_128_fp16,
    };

    static FaceRecognizers::FaceRecognizerType getFaceRecognizerOfFaceSwapper(const FaceSwapperModel &faceSwapper);
    static FaceSwapperBase *createFaceSwapper(const FaceSwapperModel &model,
                                              std::shared_ptr<FaceMaskers> maskers,
                                              const std::shared_ptr<Ort::Env> &env = nullptr);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_SWAPPER_FACE_SWAPPER_HELPER_H_
