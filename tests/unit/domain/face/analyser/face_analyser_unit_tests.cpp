#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <filesystem>
#include <cstdio>

import domain.face.analyser;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.store;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face;

using namespace domain::face;
using namespace domain::face::analyser;
using namespace domain::face::detector;
using namespace domain::face::landmarker;
using namespace domain::face::recognizer;
using namespace domain::face::classifier;
using namespace domain::face::store;
using namespace domain::ai::model_repository;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// Mock classes for isolated logic testing
class MockFaceDetector : public IFaceDetector {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(DetectionResults, detect, (const cv::Mat&), (override));
};

class MockFaceLandmarker : public IFaceLandmarker {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(LandmarkerResult, detect, (const cv::Mat&, const cv::Rect2f&), (override));
    MOCK_METHOD(domain::face::detector::Landmarks, expand_68_from_5,
                (const domain::face::detector::Landmarks&), (override));
};

class MockFaceRecognizer : public FaceRecognizer {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD((std::pair<std::vector<float>, std::vector<float>>), recognize,
                (const cv::Mat&, const domain::face::detector::Landmarks&), (override));
};

class MockFaceClassifier : public IFaceClassifier {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(ClassificationResult, classify,
                (const cv::Mat&, const domain::face::detector::Landmarks&), (override));
};

class FaceAnalyserUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        FaceStore::get_instance()->clear_faces();
        // ModelRepository is not strictly needed for pure unit tests if we mock everything,
        // but FaceAnalyser constructor might access it or check something.
        // In this case, we are injecting mocks, so FaceAnalyser shouldn't load models.

        options.model_paths.face_detector_yolo = "dummy_yolo";
        options.face_detector_options.type = DetectorType::Yolo;

        mock_detector = std::make_shared<NiceMock<MockFaceDetector>>();
        mock_landmarker = std::make_shared<NiceMock<MockFaceLandmarker>>();
        mock_recognizer = std::make_shared<NiceMock<MockFaceRecognizer>>();
        mock_classifier = std::make_shared<NiceMock<MockFaceClassifier>>();
    }

    Options options;
    std::shared_ptr<MockFaceDetector> mock_detector;
    std::shared_ptr<MockFaceLandmarker> mock_landmarker;
    std::shared_ptr<MockFaceRecognizer> mock_recognizer;
    std::shared_ptr<MockFaceClassifier> mock_classifier;
};

TEST_F(FaceAnalyserUnitTest, InitializationTest) {
    EXPECT_NO_THROW({
        FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                              mock_classifier);
    });
}
