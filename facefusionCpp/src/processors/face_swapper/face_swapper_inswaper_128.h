/**
 ******************************************************************************
 * @file           : face_swapper_inswaper_128.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_SWAPPER_FACE_SWAPPER_INSWAPER_128_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_SWAPPER_FACE_SWAPPER_INSWAPER_128_H_

#include <onnx/onnx_pb.h>
#include "face_swapper_base.h"
#include "face_helper.h"

class FaceSwapperInswaper128 : public FaceSwapperBase {
public:
    enum Version {
        V128,
        V128_fp16,
    };

    FaceSwapperInswaper128(const std::shared_ptr<Ort::Env> &env,
                           const std::shared_ptr<FaceMaskers> &faceMaskers,
                           const std::string &modelPath, const Version &version);
    ~FaceSwapperInswaper128() override = default;

    cv::Mat processFrame(const InputData *inputData) final;
    std::unordered_set<ProcessorBase::InputDataType> getInputDataTypes() final;

private:
    cv::Size m_size = cv::Size(128, 128);
    std::vector<float> m_mean = {0.0, 0.0, 0.0};
    std::vector<float> m_standardDeviation = {1.0, 1.0, 1.0};
    FaceHelper::WarpTemplateType m_warpTemplateType = FaceHelper::WarpTemplateType::Arcface_128_v2;
    int m_inputHeight;
    int m_inputWidth;
    std::vector<float> m_initializerArray;

    cv::Mat swapFace(const Face &sourceFace, const Face &targetFace, const cv::Mat &targetFrame);
    std::vector<float> prepareSourceEmbedding(const Face &sourceFace) const;
    std::vector<float> getInputImageData(const cv::Mat &cropFrame) const;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_FACE_SWAPPER_FACE_SWAPPER_INSWAPER_128_H_
