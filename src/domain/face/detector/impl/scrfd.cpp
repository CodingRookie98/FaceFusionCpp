module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <map>

module domain.face.detector;

import :impl_base;
import :types;
import :internal_creators;
import domain.face.helper;
import foundation.ai.inference_session;
import foundation.media.vision;

namespace domain::face::detector {

class Scrfd final : public FaceDetectorImplBase {
public:
    Scrfd() = default;
    ~Scrfd() override = default;

    void load_model(const std::string& model_path, const InferenceOptions& options) override {
        FaceDetectorImplBase::load_model(model_path, options);
        auto input_dims = get_input_node_dims();
        if (!input_dims.empty() && input_dims[0].size() >= 4) {
            int h = static_cast<int>(input_dims[0][2]);
            int w = static_cast<int>(input_dims[0][3]);

            // 处理动态维度或无效维度，默认为 640
            if (h > 0) m_inputHeight = h;
            else m_inputHeight = 640;

            if (w > 0) m_inputWidth = w;
            else m_inputWidth = 640;

            m_faceDetectorSize = cv::Size(m_inputWidth, m_inputHeight);
        }
    }

    DetectionResults detect(const cv::Mat& visionFrame) override;

private:
    std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat& visionFrame,
                                                            const cv::Size& faceDetectorSize);

    int m_inputHeight{640};
    int m_inputWidth{640};
    cv::Size m_faceDetectorSize{640, 640};
    float m_detectorScore = 0.5f;

    // SCRFD specific
    std::vector<int> m_featureStrides = {8, 16, 32};
    int m_anchorTotal = 2;
    int m_featureMapChannel = 1; // Assuming based on original code usage
};

std::tuple<std::vector<float>, float, float> Scrfd::preProcess(const cv::Mat& visionFrame,
                                                               const cv::Size& faceDetectorSize) {
    const int faceDetectorHeight = faceDetectorSize.height;
    const int faceDetectorWidth = faceDetectorSize.width;

    const cv::Mat tempVisionFrame = foundation::media::vision::resize_frame(
        visionFrame, cv::Size(faceDetectorWidth, faceDetectorHeight));
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

    const int imageArea = faceDetectorHeight * faceDetectorWidth;
    std::vector<float> inputData;
    inputData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputData.data(), (float*)bgrChannels[0].data, singleChnSize);
    memcpy(inputData.data() + imageArea, (float*)bgrChannels[1].data, singleChnSize);
    memcpy(inputData.data() + imageArea * 2, (float*)bgrChannels[2].data, singleChnSize);

    return std::make_tuple(inputData, ratioHeight, ratioWidth);
}

DetectionResults Scrfd::detect(const cv::Mat& visionFrame) {
    DetectionResults results;
    if (visionFrame.empty() || !is_model_loaded()) return results;

    auto [inputData, ratioHeight, ratioWidth] = preProcess(visionFrame, m_faceDetectorSize);

    std::vector<int64_t> inputImgShape = {1, 3, m_faceDetectorSize.height,
                                          m_faceDetectorSize.width};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<Ort::Value> inputTensors;
    inputTensors.reserve(1);
    inputTensors.emplace_back(
        Ort::Value::CreateTensor<float>(memory_info, inputData.data(), inputData.size(),
                                        inputImgShape.data(), inputImgShape.size()));

    std::vector<Ort::Value> ortOutputs = run(inputTensors);

    if (ortOutputs.empty()) return results;

    std::vector<cv::Rect2f> boundingBoxesRaw;
    std::vector<Landmarks> faceLandmarksRaw;
    std::vector<float> confidencesRaw;

    // Parse outputs
    // SCRFD outputs are usually 3 feature levels (8, 16, 32 strides)
    // Each level has Score, BBox, Landmarks (if enabled)
    // Original code logic:
    // Output order usually: score8, bbox8, kps8, score16, bbox16, kps16, ...
    // Check `ortOutputs` size.

    // Assuming m_featureStrides = {8, 16, 32}
    // m_featureMapChannel = 1? In original code: `ortOutputs[index + m_featureMapChannel]`
    // This implies outputs are interleaved or grouped.
    // Let's assume standard structure or follow original exactly.

    // Original loop:
    /*
    for (size_t index = 0; index < m_featureStrides.size(); ++index) {
        int size = ortOutputs[index].GetTensorTypeAndShapeInfo().GetShape()[0]; // Batch * Height *
    Width * Anchors? No, usually Flattened?
        // Original code: size is dimension 0?
        // SCRFD outputs are usually [N, H*W*A, 1] for score?

        // Original code access:
        // pdataScore = ortOutputs[index].GetTensorMutableData<float>();
        // pdataBbox = ortOutputs[index + m_featureMapChannel].GetTensorMutableData<float>();
        // pdataLandmark = ortOutputs[index + 2 *
    m_featureMapChannel].GetTensorMutableData<float>();
    }
    */

    // If m_featureStrides.size() is 3.
    // m_featureMapChannel = 3?
    // Usually SCRFD has 9 outputs (3 strides * 3 types).
    // Types: Score, BBox, KPS.
    // So indices:
    // Stride 8: 0 (score), 1 (bbox), 2 (kps)
    // Stride 16: 3 (score), 4 (bbox), 5 (kps)
    // ...
    // But original code says: `index + m_featureMapChannel`
    // If index goes 0, 1, 2.
    // If m_featureMapChannel is 3.
    // index=0: 0, 3, 6
    // index=1: 1, 4, 7
    // index=2: 2, 5, 8
    // This makes sense if outputs are [Score8, Score16, Score32, Bbox8, Bbox16, ..., Kps8...]
    // OR [Score8, Bbox8, Kps8, Score16...]

    // Let's assume 3 strides, 3 outputs per stride. Total 9 outputs.
    // If outputs are grouped by type:
    // 0,1,2 -> Scores
    // 3,4,5 -> BBoxes
    // 6,7,8 -> KPS
    // Then `m_featureMapChannel = 3`.
    // index=0 (Stride8): Score(0), Bbox(0+3=3), Kps(0+6=6).

    int featureMapChannel = 3; // Hardcoded based on common SCRFD export

    for (size_t index = 0; index < m_featureStrides.size(); ++index) {
        if (index >= ortOutputs.size()) break; // Safety

        int featureStride = m_featureStrides[index];

        // Score tensor
        auto& scoreTensor = ortOutputs[index];
        auto scoreInfo = scoreTensor.GetTensorTypeAndShapeInfo();
        int numAnchors = static_cast<int>(
            scoreInfo.GetElementCount()); // Assuming shape is [N, H*W*A, 1] or similar flattened

        // In SCRFD, shape might be [1, N_anchors, 1]

        const float* pdataScore = scoreTensor.GetTensorData<float>();
        const float* pdataBbox = ortOutputs[index + featureMapChannel].GetTensorData<float>();
        const float* pdataLandmark =
            ortOutputs[index + 2 * featureMapChannel].GetTensorData<float>();

        // Generate anchors
        int strideHeight = static_cast<int>(std::floor(m_faceDetectorSize.height / featureStride));
        int strideWidth = static_cast<int>(std::floor(m_faceDetectorSize.width / featureStride));

        // Check if numAnchors matches stride * stride * 2
        // 640/8 = 80. 80*80*2 = 12800.

        auto anchors = domain::face::helper::create_static_anchors(featureStride, m_anchorTotal,
                                                                   strideHeight, strideWidth);

        for (int i = 0; i < numAnchors; ++i) {
            float score = pdataScore[i];
            if (score >= m_detectorScore) {
                // Extract BBox
                // BBox data is [numAnchors, 4]
                float x1 = pdataBbox[i * 4 + 0] * featureStride;
                float y1 = pdataBbox[i * 4 + 1] * featureStride;
                float x2 = pdataBbox[i * 4 + 2] * featureStride;
                float y2 = pdataBbox[i * 4 + 3] * featureStride;

                cv::Rect2f bbox;
                bbox.x = x1;
                bbox.y = y1;
                bbox.width = x2 - x1;
                bbox.height = y2 - y1;

                // Extract Landmarks
                // KPS data is [numAnchors, 10]
                Landmarks kps;
                for (int k = 0; k < 5; ++k) {
                    float kx = pdataLandmark[i * 10 + k * 2 + 0] * featureStride;
                    float ky = pdataLandmark[i * 10 + k * 2 + 1] * featureStride;
                    kps.emplace_back(kx, ky);
                }

                // Anchor adjustment
                if (i < anchors.size()) {
                    bbox = domain::face::helper::distance_2_bbox(anchors[i], bbox);
                    kps = domain::face::helper::distance_2_face_landmark_5(anchors[i], kps);
                }

                // Scale back
                bbox.x *= ratioWidth;
                bbox.y *= ratioHeight;
                bbox.width *= ratioWidth;
                bbox.height *= ratioHeight;

                for (auto& p : kps) {
                    p.x *= ratioWidth;
                    p.y *= ratioHeight;
                }

                boundingBoxesRaw.push_back(bbox);
                faceLandmarksRaw.push_back(kps);
                confidencesRaw.push_back(score);
            }
        }
    }

    // NMS
    auto keepIndices = domain::face::helper::apply_nms(boundingBoxesRaw, confidencesRaw,
                                                       0.4f); // 0.4 NMS threshold

    for (int idx : keepIndices) {
        DetectionResult res;
        res.box = boundingBoxesRaw[idx];
        res.score = confidencesRaw[idx];
        res.landmarks = faceLandmarksRaw[idx];
        results.push_back(res);
    }

    return results;
}

std::unique_ptr<IFaceDetector> create_scrfd_detector() {
    return std::make_unique<Scrfd>();
}
} // namespace domain::face::detector
