#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <filesystem>

import domain.face.analyser;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.store;
import domain.ai.model_repository;
import foundation.ai.inference_session;

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

class FaceAnalyserTest : public ::testing::Test {
protected:
    void SetUp() override {
        FaceStore::get_instance()->clear_faces();
        model_repo = ModelRepository::get_instance();
        model_repo->set_model_info_file_path("./assets/models_info.json");

        std::string image_path = "./assets/standard_face_test_images/lenna.bmp";
        if (std::filesystem::exists(image_path)) { test_image = cv::imread(image_path); }

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
    std::shared_ptr<ModelRepository> model_repo;
    cv::Mat test_image;
};

TEST_F(FaceAnalyserTest, InitializationTest) {
    EXPECT_NO_THROW({
        FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                              mock_classifier);
    });
}

TEST_F(FaceAnalyserTest, GetManyFaces_MockedTest) {
    cv::Mat dummy_frame = cv::Mat::zeros(100, 100, CV_8UC3);

    DetectionResult det_res;
    det_res.box = cv::Rect2f(10, 10, 50, 50);
    det_res.score = 0.9f;
    det_res.landmarks = {cv::Point2f(20, 20), cv::Point2f(40, 20), cv::Point2f(30, 30),
                         cv::Point2f(25, 40), cv::Point2f(35, 40)};

    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));

    LandmarkerResult lm_res;
    lm_res.score = 0.9f;
    lm_res.landmarks = det_res.landmarks;
    EXPECT_CALL(*mock_landmarker, detect(_, _)).WillOnce(Return(lm_res));

    EXPECT_CALL(*mock_recognizer, recognize(_, _))
        .WillOnce(Return(std::make_pair(std::vector<float>{1.0f}, std::vector<float>{1.0f})));

    ClassificationResult class_res;
    class_res.gender = Gender::Female;
    class_res.race = Race::Asian;
    class_res.age = {20, 30};
    EXPECT_CALL(*mock_classifier, classify(_, _)).WillOnce(Return(class_res));

    FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                          mock_classifier);

    auto faces = analyser.get_many_faces(dummy_frame);

    ASSERT_EQ(faces.size(), 1);
    EXPECT_EQ(faces[0].detector_score(), 0.9f);
    EXPECT_EQ(faces[0].gender(), Gender::Female);
}

TEST_F(FaceAnalyserTest, RealImageE2ETest) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    Options real_options;
    real_options.model_paths.face_detector_scrfd = model_repo->ensure_model("scrfd");
    real_options.model_paths.face_landmarker_68by5 = model_repo->ensure_model("68_by_5");
    real_options.model_paths.face_recognizer_arcface =
        model_repo->ensure_model("arcface_w600k_r50");
    real_options.model_paths.face_classifier_fairface = model_repo->ensure_model("fairface");

    real_options.face_detector_options.type = DetectorType::SCRFD;
    real_options.face_landmarker_options.type = LandmarkerType::_68By5;
    real_options.inference_session_options =
        foundation::ai::inference_session::Options::with_best_providers();

    ASSERT_FALSE(real_options.model_paths.face_detector_scrfd.empty());

    FaceAnalyser analyser(real_options);
    auto faces = analyser.get_many_faces(test_image);

    ASSERT_FALSE(faces.empty()) << "Should detect at least one face in lenna.bmp";
    EXPECT_GT(faces[0].detector_score(), 0.5f);
    EXPECT_EQ(faces[0].kps().size(), 68); // 68by5 should result in 68 points
    EXPECT_FALSE(faces[0].embedding().empty());
}

TEST_F(FaceAnalyserTest, ModelReuseTest) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    Options opts;
    opts.model_paths.face_detector_scrfd = model_repo->ensure_model("scrfd");
    opts.face_detector_options.type = DetectorType::SCRFD;

    FaceAnalyser analyser1(opts);
    auto detector1 = analyser1.get_many_faces(test_image); // Trigger loading

    // Create a second analyser with same options
    FaceAnalyser analyser2(opts);

    // We can't directly access the private m_detector, but we can verify behavior
    // or use a friend class / test support if available.
    // For now, let's verify update_options with non-structural change.

    opts.face_detector_options.min_score = 0.6f;
    analyser1.update_options(opts);

    // If it didn't crash and still works, it's a good sign.
    auto faces = analyser1.get_many_faces(test_image);
    ASSERT_FALSE(faces.empty());
}

TEST_F(FaceAnalyserTest, CalculateFaceDistance_CalculatesCosineDistance) {
    Face face1, face2;
    face1.set_normed_embedding({1.0f, 0.0f});
    face2.set_normed_embedding({0.0f, 1.0f});

    EXPECT_FLOAT_EQ(FaceAnalyser::calculate_face_distance(face1, face2), 1.0f);

    face2.set_normed_embedding({1.0f, 0.0f});
    EXPECT_FLOAT_EQ(FaceAnalyser::calculate_face_distance(face1, face2), 0.0f);
}

TEST_F(FaceAnalyserTest, GetAverageFace_AveragesEmbeddings) {
    Face face1, face2;
    face1.set_box(cv::Rect2f(0, 0, 10, 10));
    face1.set_kps({cv::Point2f(0, 0)});
    face1.set_embedding({1.0f, 2.0f});
    face1.set_normed_embedding({0.5f, 0.5f});

    face2.set_box(cv::Rect2f(0, 0, 10, 10));
    face2.set_kps({cv::Point2f(0, 0)});
    face2.set_embedding({3.0f, 4.0f});
    face2.set_normed_embedding({0.7f, 0.7f});

    std::vector<Face> faces = {face1, face2};
    FaceAnalyser analyser(options, nullptr, nullptr, nullptr, nullptr);
    Face avg = analyser.get_average_face(faces);

    ASSERT_EQ(avg.embedding().size(), 2);
    EXPECT_FLOAT_EQ(avg.embedding()[0], 2.0f);
    EXPECT_FLOAT_EQ(avg.embedding()[1], 3.0f);

    ASSERT_EQ(avg.normed_embedding().size(), 2);
    EXPECT_FLOAT_EQ(avg.normed_embedding()[0], 0.6f);
    EXPECT_FLOAT_EQ(avg.normed_embedding()[1], 0.6f);
}

TEST_F(FaceAnalyserTest, GetManyFaces_OnDemandAnalysis_MockedTest) {
    cv::Mat frame1 = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::Mat frame2 = cv::Mat::zeros(100, 100, CV_8UC3);
    frame2.at<cv::Vec3b>(0, 0) = cv::Vec3b(1, 1, 1); // Ensure different for cache
    cv::Mat frame3 = cv::Mat::zeros(100, 100, CV_8UC3);
    frame3.at<cv::Vec3b>(0, 0) = cv::Vec3b(2, 2, 2);

    DetectionResult det_res;
    det_res.box = cv::Rect2f(10, 10, 50, 50);
    det_res.score = 0.9f;
    // 5 landmarks
    det_res.landmarks = {cv::Point2f(20, 20), cv::Point2f(40, 20), cv::Point2f(30, 30),
                         cv::Point2f(25, 40), cv::Point2f(35, 40)};

    // 1. Detection Only
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));
    EXPECT_CALL(*mock_landmarker, detect(_, _)).Times(0);
    EXPECT_CALL(*mock_recognizer, recognize(_, _)).Times(0);
    EXPECT_CALL(*mock_classifier, classify(_, _)).Times(0);

    FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                          mock_classifier);
    auto faces_det = analyser.get_many_faces(frame1, FaceAnalysisType::Detection);

    ASSERT_EQ(faces_det.size(), 1);
    EXPECT_EQ(faces_det[0].detector_score(), 0.9f);
    EXPECT_TRUE(faces_det[0].embedding().empty());

    // 2. Detection + Embedding
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));
    EXPECT_CALL(*mock_landmarker, detect(_, _)).Times(0); // Still 5 points enough? Yes usually.
    EXPECT_CALL(*mock_recognizer, recognize(_, _))
        .WillOnce(Return(std::make_pair(std::vector<float>{1.0f}, std::vector<float>{1.0f})));
    EXPECT_CALL(*mock_classifier, classify(_, _)).Times(0);

    auto faces_emb =
        analyser.get_many_faces(frame2, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);
    ASSERT_EQ(faces_emb.size(), 1);
    EXPECT_FALSE(faces_emb[0].embedding().empty());

    // 3. Detection + GenderAge
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));
    EXPECT_CALL(*mock_landmarker, detect(_, _)).Times(0);
    EXPECT_CALL(*mock_recognizer, recognize(_, _)).Times(0);

    ClassificationResult class_res;
    class_res.gender = Gender::Male;
    EXPECT_CALL(*mock_classifier, classify(_, _)).WillOnce(Return(class_res));

    auto faces_cls =
        analyser.get_many_faces(frame3, FaceAnalysisType::Detection | FaceAnalysisType::GenderAge);
    ASSERT_EQ(faces_cls.size(), 1);
    EXPECT_EQ(faces_cls[0].gender(), Gender::Male);
}

TEST_F(FaceAnalyserTest, FaceStoreSharingTest) {
    // Manually create a shared store
    auto shared_store = std::make_shared<FaceStore>();

    cv::Mat frame = cv::Mat::zeros(100, 100, CV_8UC3);
    frame.at<cv::Vec3b>(0, 0) = cv::Vec3b(5, 5, 5); // Unique frame

    DetectionResult det_res;
    det_res.box = cv::Rect2f(10, 10, 50, 50);
    det_res.score = 0.9f;
    det_res.landmarks = {cv::Point2f(20, 20), cv::Point2f(40, 20), cv::Point2f(30, 30),
                         cv::Point2f(25, 40), cv::Point2f(35, 40)};

    // Expect detection ONCE across two analysers if they share the store
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));

    // Analyser 1 with shared store
    FaceAnalyser analyser1(options, mock_detector, mock_landmarker, mock_recognizer,
                           mock_classifier, shared_store);

    auto faces1 = analyser1.get_many_faces(frame, FaceAnalysisType::Detection);
    ASSERT_EQ(faces1.size(), 1);

    // Analyser 2 with SAME shared store
    FaceAnalyser analyser2(options, mock_detector, mock_landmarker, mock_recognizer,
                           mock_classifier, shared_store);

    // Should NOT trigger detection again (served from store)
    auto faces2 = analyser2.get_many_faces(frame, FaceAnalysisType::Detection);
    ASSERT_EQ(faces2.size(), 1);
}

TEST_F(FaceAnalyserTest, CacheUpgradeTest) {
    cv::Mat frame = cv::Mat::zeros(100, 100, CV_8UC3);
    frame.at<cv::Vec3b>(0, 0) = cv::Vec3b(10, 10, 10); // Unique frame

    DetectionResult det_res;
    det_res.box = cv::Rect2f(10, 10, 50, 50);
    det_res.score = 0.9f;
    det_res.landmarks = {cv::Point2f(20, 20), cv::Point2f(40, 20), cv::Point2f(30, 30),
                         cv::Point2f(25, 40), cv::Point2f(35, 40)};

    // 1. First call: Detection Only
    // Expect detector called ONCE
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));

    FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                          mock_classifier);

    auto faces1 = analyser.get_many_faces(frame, FaceAnalysisType::Detection);
    ASSERT_EQ(faces1.size(), 1);
    EXPECT_TRUE(faces1[0].embedding().empty());

    // 2. Second call: Detection + Embedding
    // Detector should NOT be called again (cache hit for detection)
    // Recognizer SHOULD be called
    EXPECT_CALL(*mock_recognizer, recognize(_, _))
        .WillOnce(Return(std::make_pair(std::vector<float>{1.0f}, std::vector<float>{1.0f})));

    auto faces2 =
        analyser.get_many_faces(frame, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

    ASSERT_EQ(faces2.size(), 1);
    EXPECT_FALSE(faces2[0].embedding().empty());
    // Verify properties preserved from detection
    EXPECT_EQ(faces2[0].detector_score(), 0.9f);
}

TEST_F(FaceAnalyserTest, CacheMergeTest) {
    cv::Mat frame = cv::Mat::zeros(100, 100, CV_8UC3);
    frame.at<cv::Vec3b>(0, 0) = cv::Vec3b(20, 20, 20); // Unique

    DetectionResult det_res;
    det_res.box = cv::Rect2f(10, 10, 50, 50);
    det_res.score = 0.9f;
    det_res.landmarks = {cv::Point2f(20, 20), cv::Point2f(40, 20), cv::Point2f(30, 30),
                         cv::Point2f(25, 40), cv::Point2f(35, 40)};

    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(std::vector<DetectionResult>{det_res}));

    // 1. Get Embedding
    EXPECT_CALL(*mock_recognizer, recognize(_, _))
        .WillOnce(Return(std::make_pair(std::vector<float>{1.0f}, std::vector<float>{1.0f})));

    FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                          mock_classifier);
    analyser.get_many_faces(frame, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

    // 2. Get Gender (Should merge with existing Embedding)
    ClassificationResult class_res;
    class_res.gender = Gender::Female;
    EXPECT_CALL(*mock_classifier, classify(_, _)).WillOnce(Return(class_res));
    // Recognizer NOT called
    EXPECT_CALL(*mock_recognizer, recognize(_, _)).Times(0);

    auto faces_merged =
        analyser.get_many_faces(frame, FaceAnalysisType::Detection | FaceAnalysisType::GenderAge);

    ASSERT_EQ(faces_merged.size(), 1);
    EXPECT_EQ(faces_merged[0].gender(), Gender::Female); // New info
    EXPECT_FALSE(faces_merged[0].embedding().empty());   // Preserved info
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
