/**
 ******************************************************************************
 * @file           : expression_restorer.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_EXPRESSION_RESTORER_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_EXPRESSION_RESTORER_H_

#include <opencv2/opencv.hpp>
#include "expression_restorer_base.h"
#include "face_helper.h"

class ExpressionRestorer : public ExpressionRestorerBase {
public:
    ExpressionRestorer(const std::shared_ptr<Ort::Env> &env,
                       const std::shared_ptr<FaceMaskers> &faceMaskers,
                       const std::string &featureExtractorPath,
                       const std::string &motionExtractorPath,
                       const std::string &generatorPath);
    ~ExpressionRestorer() override = default;

    cv::Mat processFrame(const InputData *inputData) final;
    std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() final;
    void setRestoreFactor(float factor) {
        m_restoreFactor = factor;
    }
    cv::Mat restoreExpression(const cv::Mat &sourceFrame, const cv::Mat &targetFrame, const Face &targetFace);

private:
    cv::Size m_size{512, 512};
    FaceHelper::WarpTemplateType m_warpTemplateType = FaceHelper::Arcface_128_v2;
    float m_restoreFactor = 0.96;

    std::vector<float> getInputImageData(const cv::Mat &image);
    std::vector<float> forwardExtractFeature(const cv::Mat &image);
    std::vector<std::vector<float>> forwardExtractMotion(const cv::Mat &image);
    cv::Mat forwardGenerateFrame(std::vector<float> &featureVolume,
                                 std::vector<float> &sourceMotionPoints,
                                 std::vector<float> &targetMotionPoints);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_EXPRESSION_RESTORER_H_
