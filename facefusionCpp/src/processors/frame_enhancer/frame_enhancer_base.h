/**
 ******************************************************************************
 * @file           : frame_enhancer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-30
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_FRAME_ENHANCER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_FRAME_ENHANCER_BASE_H_

#include <onnxruntime_cxx_api.h>
#include "processor_base.h"
#include "inference_session.h"

class FrameEnhancerBase : public ProcessorBase {
public:
    FrameEnhancerBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    virtual ~FrameEnhancerBase() override = default;

    std::string getProcessorName() final;
    virtual cv::Mat processFrame(const InputData *inputData) = 0;
    virtual std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() = 0;
    ProcessorType getProcessorType() final;

    void setTileSize(const std::vector<int> &size) {
        m_tileSize = size;
    };

    void setModelScale(const int &scale) {
        m_modelScale = scale;
    };

    [[nodiscard]] int getModelScale() const {
        return m_modelScale;
    }

    void setBlend(const int &blend) {
        m_blend = blend;
    };

protected:
    Ffc::InferenceSession m_inferenceSession;
    std::vector<int> m_tileSize;
    int m_modelScale;
    int m_blend;

    [[nodiscard]] cv::Mat blendFrame(const cv::Mat &tempFrame, const cv::Mat &mergedFrame) const;
    static std::vector<float> getInputImageData(const cv::Mat &frame);
    static cv::Mat getOutputImage(const float *outputData, const cv::Size &size);
    void validateInputData(const InputData *inputData) final;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FRAME_ENHANCER_FRAME_ENHANCER_BASE_H_
