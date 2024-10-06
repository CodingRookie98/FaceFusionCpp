/**
 ******************************************************************************
 * @file           : face_enhancer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_FACE_ENHANCER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_FACE_ENHANCER_BASE_H_

#include "processor_base.h"
#include "face_maskers.h"

class FaceEnhancerBase : public ProcessorBase {
public:
    FaceEnhancerBase(const std::shared_ptr<Ort::Env> &env,
                     const std::shared_ptr<FaceMaskers> &faceMaskers,
                     const std::string &modelPath);
    virtual ~FaceEnhancerBase() = default;

    std::string getProcessorName() final;
    virtual cv::Mat processFrame(const InputData *inputData) = 0;
    virtual std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() = 0;
    ProcessorType getProcessorType() final;
    void setMaskTypes(const std::unordered_set<FaceMaskers::Type> &maskerTypes);
    void setFaceBlend(const unsigned int blend = 80);

protected:
    Ffc::InferenceSession m_inferenceSession;
    std::shared_ptr<FaceMaskers> m_faceMaskers;
    std::unordered_set<FaceMaskers::Type> m_maskerTypes;
    unsigned int m_faceBlend = 80;
    
    [[nodiscard]] cv::Mat blendFrame(const cv::Mat &targetFrame, const cv::Mat &pasteVisionFrame) const;
    void validateInputData(const InputData *inputData) final;
    
private:
    std::shared_mutex m_sharedMutex;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_ENHANCER_FACE_ENHANCER_BASE_H_
