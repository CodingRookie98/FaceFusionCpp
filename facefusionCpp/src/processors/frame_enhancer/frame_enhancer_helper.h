/**
 ******************************************************************************
 * @file           : frame_enhancer_helper.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-31
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_FRAME_ENHANCER_HELPER_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_FRAME_ENHANCER_HELPER_H_

#include "frame_enhancer_base.h"

class FrameEnhancerHelper {
public:
    enum class Model {
        Real_esrgan_x2,
        Real_esrgan_x2_fp16,
        Real_esrgan_x4,
        Real_esrgan_x4_fp16,
        Real_esrgan_x8,
        Real_esrgan_x8_fp16,
        Real_hatgan_x4,
    };

    static FrameEnhancerBase *CreateFrameEnhancer(Model model, const std::shared_ptr<Ort::Env> &env);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_FRAME_ENHANCER_HELPER_H_
