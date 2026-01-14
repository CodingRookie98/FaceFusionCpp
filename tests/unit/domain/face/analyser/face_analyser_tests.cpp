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
import domain.face;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face;
using namespace domain::face::analyser;
using namespace domain::face::detector;
using namespace domain::face::landmarker;
using namespace domain::face::recognizer;
using namespace domain::face::classifier;
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
    MOCK_METHOD(domain::face::types::Landmarks, expand_68_from_5,
                (const domain::face::types::Landmarks&), (override));
};

class MockFaceRecognizer : public FaceRecognizer {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD((std::pair<types::Embedding, types::Embedding>), recognize,
                (const cv::Mat&, const types::Landmarks&), (override));
};

class MockFaceClassifier : public IFaceClassifier {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(ClassificationResult, classify,
                (const cv::Mat&, const domain::face::types::Landmarks&), (override));
};

class FaceAnalyserTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_repo = ModelRepository::get_instance();
        model_repo->set_model_info_file_path("./assets/models_info.json");

        std::string image_path = "./assets/standard_face_test_images/lenna.bmp";
        if (std::filesystem::exists(image_path)) { test_image = cv::imread(image_path); }

        options.model_paths.face_detector_yolo = "dummy_yolo";
        options.face_detector_options.type = DetectorType::Yolo;

        mock_detector = std::make_unique<NiceMock<MockFaceDetector>>();
        mock_landmarker = std::make_unique<NiceMock<MockFaceLandmarker>>();
        mock_recognizer = std::make_unique<NiceMock<MockFaceRecognizer>>();
        mock_classifier = std::make_unique<NiceMock<MockFaceClassifier>>();
    }

    Options options;
    std::unique_ptr<MockFaceDetector> mock_detector;
    std::unique_ptr<MockFaceLandmarker> mock_landmarker;
    std::unique_ptr<MockFaceRecognizer> mock_recognizer;
    std::unique_ptr<MockFaceClassifier> mock_classifier;
    std::shared_ptr<ModelRepository> model_repo;
    cv::Mat test_image;
};

TEST_F(FaceAnalyserTest, InitializationTest) {
    EXPECT_NO_THROW({
        FaceAnalyser analyser(options, std::move(mock_detector), std::move(mock_landmarker),
                              std::move(mock_recognizer), std::move(mock_classifier));
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

    FaceAnalyser analyser(options, std::move(mock_detector), std::move(mock_landmarker),
                          std::move(mock_recognizer), std::move(mock_classifier));

    auto faces = analyser.get_many_faces(dummy_frame);

    ASSERT_EQ(faces.size(), 1);
    EXPECT_EQ(faces[0].detector_score(), 0.9f);
    EXPECT_EQ(faces[0].gender(), Gender::Female);
}

TEST_F(FaceAnalyserTest, RealImageE2ETest) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    Options real_options;
    real_options.model_paths.face_detector_scrfd = model_repo->ensure_model("face_detector_scrfd");
    real_options.model_paths.face_landmarker_68by5 =
        model_repo->ensure_model("face_landmarker_68_5");
    real_options.model_paths.face_recognizer_arcface =
        model_repo->ensure_model("face_recognizer_arcface_w600k_r50");
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
