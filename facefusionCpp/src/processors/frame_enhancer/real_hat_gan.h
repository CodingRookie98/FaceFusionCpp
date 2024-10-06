/**
 ******************************************************************************
 * @file           : real_hat_gan.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-31
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_REAL_HAT_GAN_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_REAL_HAT_GAN_H_

#include "frame_enhancer_base.h"

class RealHatGan : public FrameEnhancerBase {
public:
    RealHatGan(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~RealHatGan() override = default;

    cv::Mat processFrame(const InputData *inputData) final;
    std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() final;

private:
    [[nodiscard]] cv::Mat enhanceFrame(const cv::Mat &frame) const;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_REAL_HAT_GAN_H_
