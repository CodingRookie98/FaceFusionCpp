1 | #include < gtest / gtest.h > 2 | #include < gmock / gmock.h > 3
    | #include < opencv2 / opencv.hpp > 4 | #include < vector > 5 | #include < memory > 6
    | #include < filesystem > 7 | #include < cstdio > 8 | 9 | import domain.face.analyser;
10 | import domain.face;
11 | import domain.common;
12 | import domain.face.detector;
13 | import domain.face.landmarker;
14 | import domain.face.recognizer;
15 | import domain.face.classifier;
16 | import domain.face.store;
17 | import domain.face.selector;
18 | import domain.ai.model_repository;
19 | import foundation.ai.inference_session;
20 | 21 | using namespace domain::face;
22 | using namespace domain::face::analyser;
23 | using namespace domain::face::detector;
24 | using namespace domain::face::landmarker;
25 | using namespace domain::face::recognizer;
26 | using namespace domain::face::classifier;
27 | using namespace domain::face::store;
28 | namespace selector = domain::face::selector;
29 | using namespace domain::ai::model_repository;
30 | using ::testing::_;
31 | using ::testing::NiceMock;
32 | using ::testing::Return;
33 | 34 | // Mock classes for isolated logic testing
    35 | class MockFaceDetector : public IFaceDetector {
    36 | public : 37
        | MOCK_METHOD(void, load_model,
                      38 | (const std::string&, const foundation::ai::inference_session::Options&),
                      39 | (override));
    40 | MOCK_METHOD(DetectionResults, detect, (const cv::Mat&), (override));
    41 |
};
42 | 43 | class MockFaceLandmarker : public IFaceLandmarker {
    44 | public : 45
        | MOCK_METHOD(void, load_model,
                      46 | (const std::string&, const foundation::ai::inference_session::Options&),
                      47 | (override));
    48 | MOCK_METHOD(LandmarkerResult, detect, (const cv::Mat&, const cv::Rect2f&), (override));
    49
        | MOCK_METHOD(domain::face::detector::Landmarks, expand_68_from_5,
                      50 | (const domain::face::detector::Landmarks&), (override));
    51 |
};
52 | 53 | class MockFaceRecognizer : public FaceRecognizer {
    54 | public : 55
        | MOCK_METHOD(void, load_model,
                      56 | (const std::string&, const foundation::ai::inference_session::Options&),
                      57 | (override));
    58
        | MOCK_METHOD((std::pair<std::vector<float>, std::vector<float>>), recognize,
                      59 | (const cv::Mat&, const domain::face::detector::Landmarks&), (override));
    60 |
};
61 | 62 | class MockFaceClassifier : public IFaceClassifier {
    63 | public : 64
        | MOCK_METHOD(void, load_model,
                      65 | (const std::string&, const foundation::ai::inference_session::Options&),
                      66 | (override));
    67
        | MOCK_METHOD(ClassificationResult, classify,
                      68 | (const cv::Mat&, const domain::face::detector::Landmarks&), (override));
    69 |
};
70 | 71 | class FaceAnalyserUnitTest : public ::testing::Test {
    72 | protected : 73 | void SetUp() override {
        74 |     // Clear global store if needed, though we pass nullptr for store in constructor
                 // usually
            75 | // FaceStore::get_instance()->clear_faces();
            76 | 77 | options.model_paths.face_detector_yolo = "dummy_yolo";
        78 | options.face_detector_options.type = DetectorType::Yolo;
        79 | 80 | mock_detector = std::make_shared<NiceMock<MockFaceDetector>>();
        81 | mock_landmarker = std::make_shared<NiceMock<MockFaceLandmarker>>();
        82 | mock_recognizer = std::make_shared<NiceMock<MockFaceRecognizer>>();
        83 | mock_classifier = std::make_shared<NiceMock<MockFaceClassifier>>();
        84 |
    }
    85 | 86 | Options options;
    87 | std::shared_ptr<MockFaceDetector> mock_detector;
    88 | std::shared_ptr<MockFaceLandmarker> mock_landmarker;
    89 | std::shared_ptr<MockFaceRecognizer> mock_recognizer;
    90 | std::shared_ptr<MockFaceClassifier> mock_classifier;
    91 |
};
92 | 93 | TEST_F(FaceAnalyserUnitTest, InitializationTest) {
    94 | EXPECT_NO_THROW({
        95
            | FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                                    96 | mock_classifier);
        97 |
    });
    98 |
}
99 | 100 | TEST_F(FaceAnalyserUnitTest, GetManyFacesFullPipeline) {
    101 | // 1. Setup Mock Detector
        102 | DetectionResult det_res;
    103 | det_res.box = cv::Rect2f(10, 10, 100, 100);
    104 | det_res.score = 0.9f;
    105 | det_res.landmarks = {{10, 10}, {20, 20}, {30, 30}, {40, 40}, {50, 50}};
    106 | DetectionResults det_results = {det_res};
    107 | 108 | EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(det_results));
    109 | 110 | // 2. Setup Mock Landmarker
        111 | LandmarkerResult lm_res;
    112 | lm_res.score = 0.95f;
    113 | lm_res.landmarks.assign(68, cv::Point2f(5, 5));
    114 | EXPECT_CALL(*mock_landmarker, detect(_, _)).WillOnce(Return(lm_res));
    115 | 116 | // 3. Setup Mock Recognizer
        117 | std::pair<std::vector<float>, std::vector<float>> embedding_res;
    118 | embedding_res.first.assign(512, 0.1f);
    119 | embedding_res.second.assign(512, 0.2f);
    120 | EXPECT_CALL(*mock_recognizer, recognize(_, _)).WillOnce(Return(embedding_res));
    121 | 122 | // 4. Setup Mock Classifier
        123 | ClassificationResult cls_res;
    124 | cls_res.age.set(20, 30);
    125 | cls_res.gender = Gender::Male;
    126 | cls_res.race = Race::Asian;
    127 | EXPECT_CALL(*mock_classifier, classify(_, _)).WillOnce(Return(cls_res));
    128 | 129 | // Execute
        130
        | FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                                131 | mock_classifier);
    132 | 133 | cv::Mat dummy_frame = cv::Mat::zeros(200, 200, CV_8UC3);
    134 | auto faces = analyser.get_many_faces(dummy_frame, FaceAnalysisType::All);
    135 | 136 | // Verify
        137 | ASSERT_EQ(faces.size(), 1);
    138 | const auto& face = faces[0];
    139 | 140 | EXPECT_EQ(face.box().x, 10);
    141 | EXPECT_NEAR(face.detector_score(), 0.9f, 1e-5);
    142 | EXPECT_EQ(face.kps().size(), 68);
    143 | EXPECT_NEAR(face.landmarker_score(), 0.95f, 1e-5);
    144 | 145 | EXPECT_EQ(face.age_range().min, 20);
    146 | EXPECT_EQ(face.gender(), Gender::Male);
    147 | EXPECT_EQ(face.race(), Race::Asian);
    148 | 149 | EXPECT_EQ(face.embedding().size(), 512);
    150 |
}
151 | 152 | TEST_F(FaceAnalyserUnitTest, GetOneFaceReturnsHighestScore) {
    153 | // Setup Mock Detector returning 2 faces
        154 | DetectionResult f1;
    155 | f1.score = 0.6f;
    156 | f1.box = cv::Rect2f(0, 0, 50, 50);
    157 | f1.landmarks = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    158 | 159 | DetectionResult f2;
    160 | f2.score = 0.9f; // Higher score
    161 | f2.box = cv::Rect2f(100, 100, 50, 50);
    162 | f2.landmarks = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    163 | 164 | DetectionResults det_results = {f1, f2};
    165 | EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(det_results));
    166 | 167 | // Update options to sort by score (BestWorst)
        168 | options.face_selector_options.order = selector::Order::BestWorst;
    169 | 170 | // Execute
        171
        | FaceAnalyser analyser(options, mock_detector, mock_landmarker, mock_recognizer,
                                172 | mock_classifier);
    173 | 174 | cv::Mat dummy_frame = cv::Mat::zeros(200, 200, CV_8UC3);
    175 | // Only Detection needed for selection based on score (assuming default strategy)
        176 | auto face = analyser.get_one_face(dummy_frame, 0, FaceAnalysisType::Detection);
    177 | 178 | EXPECT_FALSE(face.is_empty());
    179 | EXPECT_NEAR(face.detector_score(), 0.9f, 1e-5);
    180 | EXPECT_EQ(face.box().x, 100);
    181 |
}
182 | 183 | TEST_F(FaceAnalyserUnitTest, CalculateFaceDistance) {
    184 | Face f1, f2;
    185 | std::vector<float> e1(512, 0.0f);
    186 | std::vector<float> e2(512, 0.0f);
    187 | 188 | e1[0] = 1.0f;
    189 | e2[0] = 1.0f; // Distance 0
    190 | 191 | f1.set_normed_embedding(e1);
    192 | f2.set_normed_embedding(e2);
    193 | 194 | float dist = FaceAnalyser::calculate_face_distance(f1, f2);
    195 | EXPECT_NEAR(dist, 0.0f, 1e-5);
    196 | 197 | e2[0] = 0.0f; // Distance 1.0 (Euclidean? Or Cosine? Usually L2)
    198 |                     // If L2: sqrt((1-0)^2 + 0...) = 1
        199 | f2.set_normed_embedding(e2);
    200 | 201 | dist = FaceAnalyser::calculate_face_distance(f1, f2);
    202 | EXPECT_GT(dist, 0.0f);
    203 |
}
204 | 205 | TEST_F(FaceAnalyserUnitTest, CompareFace) {
    206 | Face f1, f2;
    207 | // Set scores to make faces "valid" / non-empty
        208 | f1.set_detector_score(0.9f);
    209 | f2.set_detector_score(0.9f);
    210 | 211 | std::vector<float> e1(512, 0.0f);
    212 | e1[0] = 1.0f; // Make it a unit vector so dot product is 1.0
    213 | 214 | f1.set_normed_embedding(e1);
    215 | f2.set_normed_embedding(e1); // Same embedding
    216 | 217 | EXPECT_TRUE(FaceAnalyser::compare_face(f1, f2, 0.5f));
    218 |
}
219 |