/**
 ******************************************************************************
 * @file           : code_former.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-22
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_CODE_FORMER_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_CODE_FORMER_H_

#include "face_enhancer_base.h"
#include "face_helper.h"

class CodeFormer : public FaceEnhancerBase {
public:
    CodeFormer(const std::shared_ptr<Ort::Env> &env,
               const std::shared_ptr<FaceMaskers> &faceMaskers,
               const std::string &modelPath);
    ~CodeFormer() override = default;

    cv::Mat processFrame(const InputData *inputData) override;

    std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() final;

private:
    int m_inputHeight;
    int m_inputWidth;
    const cv::Size m_size = cv::Size(512, 512);
    FaceHelper::WarpTemplateType m_warpTemplateType = FaceHelper::WarpTemplateType::Ffhq_512;

    std::vector<float> getInputImageData(const cv::Mat &croppedImage) const;
    cv::Mat enhanceFace(const cv::Mat &image, const Face &targetFace) const;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_CODE_FORMER_H_
