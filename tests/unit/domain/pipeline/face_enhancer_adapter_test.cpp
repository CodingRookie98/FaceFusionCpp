#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <string>

import domain.pipeline;
import domain.face;
import domain.face.enhancer;
import domain.face.swapper;
import domain.pipeline.context;
import processor_factory;
import foundation.ai.inference_session;

using namespace domain::pipeline;
using namespace domain::face::enhancer;
using namespace domain::face::types;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// ---- T1: FaceEnhancerAdapter blend 分支行为（blend=100 全量替换，跳过 clone 不改变输出） ----

namespace {

class MockEnhancer : public IFaceEnhancer {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(cv::Mat, enhance_face, (const cv::Mat&), (override));
    MOCK_METHOD(cv::Size, get_model_input_size, (), (const, override));
};

Landmarks MakeLandmarks() {
    return {{60, 60}, {140, 60}, {100, 110}, {80, 140}, {120, 140}};
}

} // namespace

TEST(FaceEnhancerAdapterTest, BlendFullReplacesFrame) {
    domain::pipeline::register_builtin_adapters(); // 强制链接静态处理器注册
    PipelineContext ctx;
    auto enhancer = std::make_shared<NiceMock<MockEnhancer>>();
    ctx.face_enhancer = enhancer;
    ctx.inference_options = foundation::ai::inference_session::Options::with_best_providers();

    EXPECT_CALL(*enhancer, get_model_input_size()).WillRepeatedly(Return(cv::Size(512, 512)));
    EXPECT_CALL(*enhancer, enhance_face(_))
        .WillOnce(Return(cv::Mat::ones(512, 512, CV_8UC3) * 255));

    auto proc = ProcessorFactory::instance().create("face_enhancer", &ctx);
    ASSERT_NE(proc, nullptr);

    cv::Mat original = cv::Mat::zeros(256, 256, CV_8UC3);
    FrameData frame;
    frame.image = original;
    EnhanceInput input;
    input.target_faces_landmarks = {MakeLandmarks()};
    input.face_blend = 100; // 全量替换（无需原始帧副本）
    frame.enhance_input = std::move(input);

    proc->process(frame);

    // blend=100 → 帧被增强结果替换（含白色像素 ≠ 原黑图）
    cv::Mat diff;
    cv::absdiff(frame.image, original, diff);
    EXPECT_GT(cv::norm(diff, cv::NORM_L1), 0) << "blend=100 应全量替换帧内容";
}

TEST(FaceEnhancerAdapterTest, BlendPartialWeighted) {
    domain::pipeline::register_builtin_adapters();
    PipelineContext ctx;
    auto enhancer = std::make_shared<NiceMock<MockEnhancer>>();
    ctx.face_enhancer = enhancer;
    ctx.inference_options = foundation::ai::inference_session::Options::with_best_providers();

    EXPECT_CALL(*enhancer, get_model_input_size()).WillRepeatedly(Return(cv::Size(512, 512)));
    EXPECT_CALL(*enhancer, enhance_face(_))
        .WillOnce(Return(cv::Mat::ones(512, 512, CV_8UC3) * 255));

    auto proc = ProcessorFactory::instance().create("face_enhancer", &ctx);
    ASSERT_NE(proc, nullptr);

    cv::Mat original = cv::Mat::zeros(256, 256, CV_8UC3);
    FrameData frame;
    frame.image = original;
    EnhanceInput input;
    input.target_faces_landmarks = {MakeLandmarks()};
    input.face_blend = 50; // 加权混合
    frame.enhance_input = std::move(input);

    proc->process(frame);

    // blend=50 → 输出为混合（非全量替换）：增强区域像素 < 255（加权 0.5）
    cv::Mat diff;
    cv::absdiff(frame.image, original, diff);
    EXPECT_GT(cv::norm(diff, cv::NORM_L1), 0);
    // 增强区域均值应低于 blend=100 场景（加权衰减）——取帧中心采样验证
    const auto kCenter = frame.image.at<cv::Vec3b>(128, 128);
    EXPECT_LT(kCenter[0], 255);
}