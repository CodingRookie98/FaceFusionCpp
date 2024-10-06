/**
 ******************************************************************************
 * @file           : exprssion_restorer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_EXPRESSION_RESTORER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_EXPRESSION_RESTORER_BASE_H_

#include "processor_base.h"
#include "face_maskers.h"

class ExpressionRestorerBase : public ProcessorBase {
public:
    ExpressionRestorerBase(const std::shared_ptr<Ort::Env> &env,
                           const std::shared_ptr<FaceMaskers> &faceMaskers,
                           const std::string &featureExtractorPath,
                           const std::string &motionExtractorPath,
                           const std::string &generatorPath);
    virtual ~ExpressionRestorerBase() override = default;

    std::string getProcessorName() final;
    virtual cv::Mat processFrame(const InputData *inputData) = 0;
    virtual std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() = 0;
    ProcessorType getProcessorType() final;
    void setMaskTypes(const std::unordered_set<FaceMaskers::Type> &maskerTypes);

protected:
    Ffc::InferenceSession m_featureSession;
    Ffc::InferenceSession m_motionSession;
    Ffc::InferenceSession m_generatorSession;
    std::shared_ptr<FaceMaskers> m_faceMaskers;
    std::unordered_set<FaceMaskers::Type> m_maskerTypes;
    void validateInputData(const InputData *inputData) final;

private:
    std::shared_mutex m_sharedMutex;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_EXPRESSION_RESTORER_BASE_H_
