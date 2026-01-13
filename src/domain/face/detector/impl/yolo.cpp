module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <tuple>
#include <vector>
#include <algorithm>

module domain.face.detector;

import :impl_base;
import :types;
import :internal_creators;
import foundation.ai.inference_session;
import foundation.media.vision;

namespace domain::face::detector {

class Yolo final : public FaceDetectorImplBase {
public:
    Yolo() = default;
    ~Yolo() override = default;

    void load_model(const std::string& model_path, const InferenceOptions& options) override {
        InferenceSession::load_model(model_path, options);
        auto input_dims = get_input_node_dims();
        if (!input_dims.empty() && input_dims[0].size() >= 4) {
            // NCHW: [1, 3, 640, 640]
            input_height_ = static_cast<int>(input_dims[0][2]);
            input_width_ = static_cast<int>(input_dims[0][3]);
            faceDetectorSize_ = cv::Size(input_width_, input_height_);
        }
    }

    DetectionResults detect(const cv::Mat& visionFrame) override;

private:
    static std::tuple<std::vector<float>, float, float> preProcess(
        const cv::Mat& visionFrame, const cv::Size& faceDetectorSize);

    int input_height_{640};
    int input_width_{640};
    cv::Size faceDetectorSize_{640, 640};
    float score_threshold_ = 0.5f;
};

std::tuple<std::vector<float>, float, float> Yolo::preProcess(const cv::Mat& visionFrame,
                                                              const cv::Size& faceDetectorSize) {
    const int faceDetectorHeight = faceDetectorSize.height;
    const int faceDetectorWidth = faceDetectorSize.width;

    cv::Mat tempVisionFrame =
        foundation::media::vision::resize_frame(visionFrame, faceDetectorSize);
    float ratioHeight =
        static_cast<float>(visionFrame.rows) / static_cast<float>(tempVisionFrame.rows);
    float ratioWidth =
        static_cast<float>(visionFrame.cols) / static_cast<float>(tempVisionFrame.cols);

    cv::Mat detectVisionFrame = cv::Mat::zeros(faceDetectorHeight, faceDetectorWidth, CV_32FC3);
    tempVisionFrame.copyTo(
        detectVisionFrame(cv::Rect(0, 0, tempVisionFrame.cols, tempVisionFrame.rows)));

    std::vector<cv::Mat> bgrChannels(3);
    cv::split(detectVisionFrame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / 128.0, -127.5 / 128.0);
    }

    const int imageArea = faceDetectorWidth * faceDetectorHeight;
    std::vector<float> inputData;
    inputData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputData.data(), (float*)bgrChannels[0].data, singleChnSize);
    memcpy(inputData.data() + imageArea, (float*)bgrChannels[1].data, singleChnSize);
    memcpy(inputData.data() + imageArea * 2, (float*)bgrChannels[2].data, singleChnSize);
    return std::make_tuple(inputData, ratioHeight, ratioWidth);
}

DetectionResults Yolo::detect(const cv::Mat& visionFrame) {
    DetectionResults results;
    if (visionFrame.empty()) {
        return results; // Or throw
    }

    if (!is_model_loaded()) { return results; }

    // Check support sizes? Yolo supports 640x640 only for now as per original code

    auto [inputData, ratioHeight, ratioWidth] = preProcess(visionFrame, faceDetectorSize_);

    const std::vector<int64_t> inputImgShape{1, 3, faceDetectorSize_.height,
                                             faceDetectorSize_.width};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<Ort::Value> inputTensors;
    inputTensors.reserve(1);
    inputTensors.emplace_back(
        Ort::Value::CreateTensor<float>(memory_info, inputData.data(), inputData.size(),
                                        inputImgShape.data(), inputImgShape.size()));

    std::vector<Ort::Value> ortOutputs = run(inputTensors);

    if (ortOutputs.empty()) return results;

    float* pdata = ortOutputs[0].GetTensorMutableData<float>();
    const int numBox = static_cast<int>(ortOutputs[0].GetTensorTypeAndShapeInfo().GetShape()[2]);

    for (int i = 0; i < numBox; i++) {
        const float score = pdata[4 * numBox + i];
        if (score > score_threshold_) {
            float xmin = (pdata[i] - 0.5f * pdata[2 * numBox + i]) * ratioWidth;
            float ymin = (pdata[numBox + i] - 0.5f * pdata[3 * numBox + i]) * ratioHeight;
            float xmax = (pdata[i] + 0.5f * pdata[2 * numBox + i]) * ratioWidth;
            float ymax = (pdata[numBox + i] + 0.5f * pdata[3 * numBox + i]) * ratioHeight;

            xmin = std::max(0.0f, std::min(xmin, static_cast<float>(visionFrame.cols)));
            ymin = std::max(0.0f, std::min(ymin, static_cast<float>(visionFrame.rows)));
            xmax = std::max(0.0f, std::min(xmax, static_cast<float>(visionFrame.cols)));
            ymax = std::max(0.0f, std::min(ymax, static_cast<float>(visionFrame.rows)));

            DetectionResult result;
            result.box = cv::Rect2f(xmin, ymin, xmax - xmin, ymax - ymin);
            result.score = score;

            // Landmarks
            for (int j = 5; j < 20; j += 3) {
                cv::Point2f point2F;
                point2F.x = pdata[j * numBox + i] * ratioWidth;
                point2F.y = pdata[(j + 1) * numBox + i] * ratioHeight;
                result.landmarks.emplace_back(point2F);
            }

            results.emplace_back(std::move(result));
        }
    }

    return results;
}

std::unique_ptr<IFaceDetector> create_yolo_detector() {
    return std::make_unique<Yolo>();
}
} // namespace domain::face::detector
