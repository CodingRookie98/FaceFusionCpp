/**
 ******************************************************************************
 * @file           : face_enhancer_helper.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-21
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_FACE_ENHANCER_HELPER_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_FACE_ENHANCER_HELPER_H_

#include "model_manager.h"
#include "face_enhancer_base.h"

class FaceEnhancerHelper {
public:
    enum Model {
        Gfpgan_12 = Ffc::ModelManager::Model::Gfpgan_12,
        Gfpgan_13 = Ffc::ModelManager::Model::Gfpgan_13,
        Gfpgan_14 = Ffc::ModelManager::Model::Gfpgan_14,
        _CodeFormer = Ffc::ModelManager::Model::Codeformer,
    };

    static FaceEnhancerBase *createFaceEnhancer(const FaceEnhancerHelper::Model &model,
                                                const std::shared_ptr<FaceMaskers>& maskers,
                                                const std::shared_ptr<Ort::Env> &env = nullptr);

private:
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_FACE_ENHANCER_HELPER_H_
