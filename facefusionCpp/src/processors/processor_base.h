/**
 ******************************************************************************
 * @file           : processor_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-20
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_PROCESSORS_FRAME_MODULES_PROCESSOR_H_
#define FACEFUSIONCPP_SRC_PROCESSORS_FRAME_MODULES_PROCESSOR_H_

#include <unordered_set>
#include <string>
#include "face.h"

class ProcessorBase {
public:
    ProcessorBase() = default;
    virtual ~ProcessorBase() = default;

    class InputData {
    public:
        std::vector<Face> *m_sourceFaces = nullptr;
        cv::Mat *m_originalTargetFrame = nullptr;
        std::vector<Face> *m_targetFaces = nullptr;
        cv::Mat *m_targetFrame = nullptr;
        ~InputData() {
            delete m_sourceFaces;
            delete m_originalTargetFrame;
            delete m_targetFaces;
            delete m_targetFrame;
        }
    };
    enum InputDataType {
        SourceFaces,
        OriginalTargetFrame,
        TargetFaces,
        TargetFrame
    };
    enum ProcessorType {
        FaceSwapper,
        FaceEnhancer,
        ExpressionRestorer,
        FrameEnhancer,
    };

    virtual cv::Mat processFrame(const InputData *inputData) = 0;
    virtual std::string getProcessorName() = 0;
    virtual std::unordered_set<InputDataType> getInputDataTypes() = 0;
    virtual ProcessorType getProcessorType() = 0;

protected:
    virtual void validateInputData(const InputData *inputData) = 0;
};

#endif // FACEFUSIONCPP_SRC_PROCESSORS_FRAME_MODULES_PROCESSOR_BASE_H_