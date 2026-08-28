#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <unordered_set>
#include <string>

import domain.pipeline;
import domain.face;
import domain.face.analyser;
import domain.face.detector;
import domain.face.masker;
import config.task;
import services.pipeline.processors.face_analysis;
import foundation.ai.inference_session;

using namespace domain::face;
using namespace domain::face::analyser;
using namespace domain::face::detector;
using namespace domain::face::masker;
using namespace domain::pipeline;
using namespace config;
using namespace services::pipeline::processors;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// ---- T1: mask 链路接线 + 共享 ----

namespace {

class MockDetector : public IFaceDetector {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(DetectionResults, detect, (const cv::Mat&), (override));
};

class MockOccluder : public IFaceOccluder {
public:
    MOCK_METHOD(cv::Mat, create_occlusion_mask, (const cv::Mat&), (override));
};

class MockRegionMasker : public IFaceRegionMasker {
public:
    MOCK_METHOD(cv::Mat, create_region_mask,
                (const cv::Mat&, const std::unordered_set<domain::face::types::FaceRegion>&),
                (override));
};

DetectionResults MakeOneDetection() {
    DetectionResult det;
    det.box = cv::Rect2f(50, 50, 100, 100);
    det.score = 0.9f;
    det.landmarks = {{60, 60}, {140, 60}, {100, 110}, {80, 140}, {120, 140}};
    return {det};
}

} // namespace

class FaceAnalysisMaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        options.face_detector_options.type = DetectorType::Yolo;
        mock_detector = std::make_shared<NiceMock<MockDetector>>();
        mock_occluder = std::make_shared<NiceMock<MockOccluder>>();
        mock_region = std::make_shared<NiceMock<MockRegionMasker>>();
    }

    Options options;
    std::shared_ptr<MockDetector> mock_detector;
    std::shared_ptr<MockOccluder> mock_occluder;
    std::shared_ptr<MockRegionMasker> mock_region;
};

TEST_F(FaceAnalysisMaskTest, MaskComputedOnceAndShared) {
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(MakeOneDetection()));
    // 1 张脸 → occluder/region 各调用 1 次
    EXPECT_CALL(*mock_occluder, create_occlusion_mask(_))
        .Times(1)
        .WillOnce(Return(cv::Mat::zeros(256, 256, CV_8UC1)));
    EXPECT_CALL(*mock_region, create_region_mask(_, _))
        .Times(1)
        .WillOnce(Return(cv::Mat::zeros(512, 512, CV_8UC1)));

    auto analyser =
        std::make_shared<FaceAnalyser>(options, mock_detector, nullptr, nullptr, nullptr);
    FaceMaskerConfig masker_cfg;
    masker_cfg.types = {"box", "occlusion", "region"};

    FaceAnalysisProcessor proc(analyser, nullptr, FaceAnalysisRequirements{}, nullptr, masker_cfg,
                               mock_occluder, mock_region);

    FrameData frame;
    frame.image = cv::Mat::zeros(211, 200, CV_8UC3);
    proc.process(frame);

    ASSERT_TRUE(frame.mask_cache.has_value()) << "启用 occlusion/region 时应生成共享 mask";
    ASSERT_EQ(frame.mask_cache->masks.size(), 1u);
    EXPECT_EQ(frame.mask_cache->reference_size, 512);
}

TEST_F(FaceAnalysisMaskTest, MaskSkippedWhenBoxOnly) {
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(MakeOneDetection()));
    // 默认 box only → 分割模型不调用
    EXPECT_CALL(*mock_occluder, create_occlusion_mask(_)).Times(0);
    EXPECT_CALL(*mock_region, create_region_mask(_, _)).Times(0);

    auto analyser =
        std::make_shared<FaceAnalyser>(options, mock_detector, nullptr, nullptr, nullptr);
    FaceMaskerConfig masker_cfg;
    masker_cfg.types = {"box"};

    FaceAnalysisProcessor proc(analyser, nullptr, FaceAnalysisRequirements{}, nullptr, masker_cfg,
                               mock_occluder, mock_region);

    FrameData frame;
    frame.image = cv::Mat::zeros(212, 200, CV_8UC3);
    proc.process(frame);

    EXPECT_FALSE(frame.mask_cache.has_value()) << "默认 box 保底：零额外推理";
}

TEST_F(FaceAnalysisMaskTest, MaskSkippedWhenNoMaskersProvided) {
    EXPECT_CALL(*mock_detector, detect(_)).WillOnce(Return(MakeOneDetection()));

    auto analyser =
        std::make_shared<FaceAnalyser>(options, mock_detector, nullptr, nullptr, nullptr);
    FaceMaskerConfig masker_cfg;
    masker_cfg.types = {"box", "occlusion", "region"};

    // 未提供 occluder/region_masker → 即使配置启用也不计算（无模型可用）
    FaceAnalysisProcessor proc(analyser, nullptr, FaceAnalysisRequirements{}, nullptr, masker_cfg,
                               nullptr, nullptr);

    FrameData frame;
    frame.image = cv::Mat::zeros(213, 200, CV_8UC3);
    proc.process(frame);

    EXPECT_FALSE(frame.mask_cache.has_value());
}