#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <opencv2/core/mat.hpp>

import domain.pipeline.context;
import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import domain.face.masker;
import domain.frame.enhancer;
import foundation.ai.inference_session;

using namespace domain::pipeline;
using namespace foundation::ai::inference_session;

// --- Mock Classes (Minimal for Ptr testing) ---

class MockFaceSwapper : public domain::face::swapper::IFaceSwapper {
public:
    void load_model(const std::string&, const foundation::ai::inference_session::Options&) override {}
    // Implement only the pure virtual method from IFaceSwapper
    cv::Mat swap_face(cv::Mat target_crop, const std::vector<float>&) override { return target_crop; }
    cv::Size get_model_input_size() const override { return {128, 128}; }
    ~MockFaceSwapper() override = default;
};

class MockFaceEnhancer : public domain::face::enhancer::IFaceEnhancer {
public:
    void load_model(const std::string&, const foundation::ai::inference_session::Options&) override {}
    // Implement only the pure virtual method from IFaceEnhancer
    cv::Mat enhance_face(const cv::Mat& target_crop) override { return target_crop; }
    cv::Size get_model_input_size() const override { return {512, 512}; }
    ~MockFaceEnhancer() override = default;
};

// --- Tests ---

TEST(PipelineContextTest, DefaultConstruction) {
    PipelineContext ctx;
    
    // Pointers should be null by default
    EXPECT_EQ(ctx.swapper, nullptr);
    EXPECT_EQ(ctx.face_enhancer, nullptr);
    EXPECT_EQ(ctx.restorer, nullptr);
    EXPECT_EQ(ctx.occluder, nullptr);
    EXPECT_EQ(ctx.region_masker, nullptr);
    
    // Strings should be empty
    EXPECT_TRUE(ctx.swapper_model_path.empty());
    EXPECT_TRUE(ctx.enhancer_model_path.empty());
}

TEST(PipelineContextTest, MemberAssignment) {
    PipelineContext ctx;
    
    auto swapper = std::make_shared<MockFaceSwapper>();
    ctx.swapper = swapper;
    EXPECT_EQ(ctx.swapper, swapper);
    
    auto enhancer = std::make_shared<MockFaceEnhancer>();
    ctx.face_enhancer = enhancer;
    EXPECT_EQ(ctx.face_enhancer, enhancer);
    
    ctx.swapper_model_path = "/path/to/swapper";
    EXPECT_EQ(ctx.swapper_model_path, "/path/to/swapper");
    
    ctx.inference_options.execution_providers = {ExecutionProvider::CUDA};
    EXPECT_TRUE(ctx.inference_options.execution_providers.contains(ExecutionProvider::CUDA));
}

TEST(PipelineContextTest, FactoryFunctionAssignment) {
    PipelineContext ctx;
    bool factory_called = false;
    
    ctx.frame_enhancer_factory = [&]() -> std::shared_ptr<domain::frame::enhancer::IFrameEnhancer> {
        factory_called = true;
        return nullptr;
    };
    
    ASSERT_TRUE(ctx.frame_enhancer_factory);
    ctx.frame_enhancer_factory();
    EXPECT_TRUE(factory_called);
}

TEST(PipelineContextTest, CopyBehavior) {
    // PipelineContext contains shared_ptrs, so copy should share ownership
    PipelineContext ctx1;
    auto swapper = std::make_shared<MockFaceSwapper>();
    ctx1.swapper = swapper;
    ctx1.swapper_model_path = "model.onnx";
    
    PipelineContext ctx2 = ctx1;
    
    EXPECT_EQ(ctx2.swapper, swapper);
    EXPECT_EQ(ctx2.swapper.use_count(), 3); // swapper local + ctx1.swapper + ctx2.swapper
    EXPECT_EQ(ctx2.swapper_model_path, "model.onnx");
}
