#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <filesystem>
#include <cstdio>

import domain.face.analyser;
import domain.face;
import domain.common;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.store;
import domain.face.selector;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face;
using namespace domain::face::analyser;
using namespace domain::face::detector;
using namespace domain::face::landmarker;
using namespace domain::face::recognizer;
using namespace domain::face::classifier;
using namespace domain::face::store;
namespace selector = domain::face::selector;
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
        // Clear global store if needed, though we pass nullptr for store in constructor usually
        // FaceStore::get_instance()->clear_faces();

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

TEST_F(FaceAnalyserUnitTest, GetManyFaces_FullPipeline) {
    // 1. Setup Mock Detector
    DetectionResult det_res;
    det_res.box = cv::Rect2f(10, 10, 100, 100);
    det_res.score = 0.9f;
    det_res.landmarks = { {10,10}, {20,20}, {30,30}, {40,40}, {50,50} }; 
    DetectionResults det_results = { det_res };

    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(det_results));

    // 2. Setup Mock Landmarker
    LandmarkerResult lm_res;
    lm_res.score = 0.95f;
    lm_res.landmarks.assign(68, cv::Point2f(5,5)); 
    EXPECT_CALL(*mock_landmarker, detect(_, _)).WillOnce(Return(lm_res));

    // 3. Setup Mock Recognizer
    std::pair<std::vector<float>, std::vector<float>> embedding_res;
    embedding_res.first.assign(512, 0.1f);
    embedding_res.second.assign(512, 0.2f);
    EXPECT_CALL(*mock_recognizer, recognize(_, _)).WillOnce(Return(embedding_res));
    
    // 4. Setup Mock Classifier
    ClassificationResult cls_res;
    cls_res.age.set(20, 30);
    cls_res.gender = Gender::Male;
    cls_res.race = Race::Asian;
    EXPECT_CALL(*mock_classifier, classify(_, _)).WillOnce(Return(cls_res));

    // Execute
    FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer, mock_classifier);
    
    cv::Mat dummy_frame = cv::Mat::zeros(200, 200, CV_8UC3);
    auto faces = analyser.get_many_faces(dummy_frame, FaceAnalysisType::All);

    // Verify
    ASSERT_EQ(faces.size(), 1);
    const auto& face = faces[0];
    
    EXPECT_EQ(face.box().x, 10);
    EXPECT_NEAR(face.detector_score(), 0.9f, 1e-5);
    EXPECT_EQ(face.kps().size(), 68); 
    EXPECT_NEAR(face.landmarker_score(), 0.95f, 1e-5);
    
    EXPECT_EQ(face.age_range().min, 20);
    EXPECT_EQ(face.gender(), Gender::Male);
    EXPECT_EQ(face.race(), Race::Asian);
    
    EXPECT_EQ(face.embedding().size(), 512);
}

TEST_F(FaceAnalyserUnitTest, GetOneFace_ReturnsHighestScore) {
    // Setup Mock Detector returning 2 faces
    DetectionResult f1;
    f1.score = 0.6f;
    f1.box = cv::Rect2f(0,0,50,50);
    f1.landmarks = {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}};
    
    DetectionResult f2;
    f2.score = 0.9f; // Higher score
    f2.box = cv::Rect2f(100,100,50,50);
    f2.landmarks = {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}};

    DetectionResults det_results = { f1, f2 };
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(det_results));

    // Update options to sort by score (BestWorst)
    options.face_selector_options.order = selector::Order::BestWorst;

    // Execute
    FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer, mock_classifier);
    
    cv::Mat dummy_frame = cv::Mat::zeros(200, 200, CV_8UC3);
    // Only Detection needed for selection based on score (assuming default strategy)
    auto face = analyser.get_one_face(dummy_frame, 0, FaceAnalysisType::Detection);

    EXPECT_FALSE(face.is_empty());
    EXPECT_NEAR(face.detector_score(), 0.9f, 1e-5);
    EXPECT_EQ(face.box().x, 100);
}

TEST_F(FaceAnalyserUnitTest, CalculateFaceDistance) {
    Face f1, f2;
    std::vector<float> e1(512, 0.0f);
    std::vector<float> e2(512, 0.0f);
    
    e1[0] = 1.0f;
    e2[0] = 1.0f; // Distance 0
    
    f1.set_normed_embedding(e1);
    f2.set_normed_embedding(e2);
    
    float dist = FaceAnalyser::calculate_face_distance(f1, f2);
    EXPECT_NEAR(dist, 0.0f, 1e-5);
    
    e2[0] = 0.0f; // Distance 1.0 (Euclidean? Or Cosine? Usually L2)
    // If L2: sqrt((1-0)^2 + 0...) = 1
    f2.set_normed_embedding(e2);
    
    dist = FaceAnalyser::calculate_face_distance(f1, f2);
    EXPECT_GT(dist, 0.0f);
}

TEST_F(FaceAnalyserUnitTest, CompareFace) {
    Face f1, f2;
    // Set scores to make faces "valid" / non-empty
    f1.set_detector_score(0.9f);
    f2.set_detector_score(0.9f);

    std::vector<float> e1(512, 0.0f);
    e1[0] = 1.0f; // Make it a unit vector so dot product is 1.0
    
    f1.set_normed_embedding(e1);
    f2.set_normed_embedding(e1); // Same embedding
    
    EXPECT_TRUE(FaceAnalyser::compare_face(f1, f2, 0.5f));
}
