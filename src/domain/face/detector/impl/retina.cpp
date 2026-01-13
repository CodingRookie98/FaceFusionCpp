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

    class Retina final : public FaceDetectorImplBase {
    public:
        Retina() = default;
        ~Retina() override = default;

        void load_model(const std::string& model_path, const InferenceOptions& options) override {
            InferenceSession::load_model(model_path, options);
            auto input_dims = get_input_node_dims();
            if (!input_dims.empty() && input_dims[0].size() >= 4) {
                 m_inputHeight = static_cast<int>(input_dims[0][2]);
                 m_inputWidth = static_cast<int>(input_dims[0][3]);
                 m_faceDetectorSize = cv::Size(m_inputWidth, m_inputHeight);
            }
        }

        DetectionResults detect(const cv::Mat& visionFrame) override;

    private:
        std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize);
        
        int m_inputHeight{640};
        int m_inputWidth{640};
        cv::Size m_faceDetectorSize{640, 640};
        float m_detectorScore = 0.5f;
        
        // Retina specific
        std::vector<int> m_featureStrides = {8, 16, 32};
        int m_anchorTotal = 2;
        int m_featureMapChannel = 3; 
    };

    std::tuple<std::vector<float>, float, float>
    Retina::preProcess(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize) {
        const int faceDetectorHeight = faceDetectorSize.height;
        const int faceDetectorWidth = faceDetectorSize.width;

        const cv::Mat tempVisionFrame = foundation::media::vision::resize_frame(visionFrame, cv::Size(faceDetectorWidth, faceDetectorHeight));
        float ratioHeight = static_cast<float>(visionFrame.rows) / static_cast<float>(tempVisionFrame.rows);
        float ratioWidth = static_cast<float>(visionFrame.cols) / static_cast<float>(tempVisionFrame.cols);

        cv::Mat detectVisionFrame = cv::Mat::zeros(faceDetectorHeight, faceDetectorWidth, CV_32FC3);
        tempVisionFrame.copyTo(detectVisionFrame(cv::Rect(0, 0, tempVisionFrame.cols, tempVisionFrame.rows)));

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

    DetectionResults Retina::detect(const cv::Mat& visionFrame) {
        DetectionResults results;
        if (visionFrame.empty() || !is_model_loaded()) return results;

        auto [inputData, ratioHeight, ratioWidth] = preProcess(visionFrame, m_faceDetectorSize);

        std::vector<int64_t> inputImgShape = {1, 3, m_faceDetectorSize.height, m_faceDetectorSize.width};
        
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        std::vector<Ort::Value> inputTensors;
        inputTensors.reserve(1);
        inputTensors.emplace_back(Ort::Value::CreateTensor<float>(memory_info, inputData.data(), inputData.size(), inputImgShape.data(), inputImgShape.size()));
        
        std::vector<Ort::Value> ortOutputs = run(inputTensors);

        if (ortOutputs.empty()) return results;

        std::vector<cv::Rect2f> boundingBoxesRaw;
        std::vector<Landmarks> faceLandmarksRaw;
        std::vector<float> confidencesRaw;

        for (size_t index = 0; index < m_featureStrides.size(); ++index) {
             if (index >= ortOutputs.size()) break;
             
             int featureStride = m_featureStrides[index];
             
             auto& outputTensor = ortOutputs[index]; // Score?
             // RetinaFace outputs structure depends on export.
             // Original code uses: 
             // ortOutputs[index]: Score?
             // ortOutputs[index + m_featureMapChannel]: BBox?
             // ortOutputs[index + 2 * m_featureMapChannel]: Landmark?
             // Assuming interleaved outputs similar to SCRFD.
             
             const float* pdataScore = ortOutputs[index].GetTensorData<float>();
             const float* pdataBbox = ortOutputs[index + m_featureMapChannel].GetTensorData<float>();
             const float* pdataLandmark = ortOutputs[index + 2 * m_featureMapChannel].GetTensorData<float>();
             
             size_t numAnchors = ortOutputs[index].GetTensorTypeAndShapeInfo().GetElementCount(); // Typically flattened [N*H*W*A]
             
             // In RetinaFace, score shape might include channel=2 (fg/bg) or just 1.
             // Usually RetinaFace has 2 channels for score (bg, fg).
             // But element count is total elements.
             // Let's check original code:
             // int size = ortOutputs[index].GetTensorTypeAndShapeInfo().GetShape()[0]; -> This looks like 1st dim if flattened?
             // Or maybe [Batch, Size, ...]
             
             int strideHeight = static_cast<int>(std::floor(m_faceDetectorSize.height / featureStride));
             int strideWidth = static_cast<int>(std::floor(m_faceDetectorSize.width / featureStride));
             
             auto anchors = domain::face::helper::create_static_anchors(featureStride, m_anchorTotal, strideHeight, strideWidth);
             
             // Assuming pdataScore has 1 value per anchor (confidence of face)
             // If it has 2 values (bg, fg), we need to stride by 2?
             // Original code: float tempScore = *(pdataScoreRaw + j);
             // It implies 1 value per anchor.
             
             for (size_t j = 0; j < anchors.size() && j < numAnchors; ++j) {
                 float score = pdataScore[j];
                 if (score >= m_detectorScore) {
                     // Extract BBox
                     // BBox: 4 values per anchor
                     float x1 = pdataBbox[j * 4 + 0] * featureStride;
                     float y1 = pdataBbox[j * 4 + 1] * featureStride;
                     float x2 = pdataBbox[j * 4 + 2] * featureStride;
                     float y2 = pdataBbox[j * 4 + 3] * featureStride;
                     
                     cv::Rect2f bbox;
                     bbox.x = x1;
                     bbox.y = y1;
                     bbox.width = x2 - x1;
                     bbox.height = y2 - y1;
                     
                     // Extract Landmarks
                     // 10 values per anchor (5 points x 2)
                     Landmarks kps;
                     for (int k = 0; k < 5; ++k) {
                         float kx = pdataLandmark[j * 10 + k * 2 + 0] * featureStride;
                         float ky = pdataLandmark[j * 10 + k * 2 + 1] * featureStride;
                         kps.emplace_back(kx, ky);
                     }
                     
                     // Adjust
                     bbox = domain::face::helper::distance_2_bbox(anchors[j], bbox);
                     kps = domain::face::helper::distance_2_face_landmark_5(anchors[j], kps);
                     
                     // Scale
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
        auto keepIndices = domain::face::helper::apply_nms(boundingBoxesRaw, confidencesRaw, 0.4f);

        for (int idx : keepIndices) {
            DetectionResult res;
            res.box = boundingBoxesRaw[idx];
            res.score = confidencesRaw[idx];
            res.landmarks = faceLandmarksRaw[idx];
            results.push_back(res);
        }

        return results;
    }

    std::unique_ptr<IFaceDetector> create_retina_detector() {
        return std::make_unique<Retina>();
    }
}
