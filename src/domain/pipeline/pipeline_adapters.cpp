module;
#include <memory>
#include <string>
#include <stdexcept>
#include <iostream>

module domain.pipeline;

import domain.pipeline.context;
import processor_factory;

using namespace domain::pipeline;

// Factory registration helpers
namespace {
// Cast helper no longer needed as factory now takes typed PipelineContext*
}

// Registration
static domain::pipeline::ProcessorRegistrar register_swapper("face_swapper", [](const void* ptr) {
    const auto* context = static_cast<const PipelineContext*>(ptr);
    return SwapperAdapter::create(context);
});

static domain::pipeline::ProcessorRegistrar register_face_enhancer(
    "face_enhancer", [](const void* ptr) {
        const auto* context = static_cast<const PipelineContext*>(ptr);
        return FaceEnhancerAdapter::create(context);
    });

static domain::pipeline::ProcessorRegistrar register_expression(
    "expression_restorer", [](const void* ptr) {
        const auto* context = static_cast<const PipelineContext*>(ptr);
        return ExpressionAdapter::create(context);
    });

static domain::pipeline::ProcessorRegistrar register_frame_enhancer(
    "frame_enhancer", [](const void* ptr) {
        const auto* context = static_cast<const PipelineContext*>(ptr);
        return FrameEnhancerAdapter::create(context);
    });

namespace domain::pipeline {

void register_builtin_adapters() {
    // This function doesn't need to do anything, but calling it forces linkage
    // of this translation unit, ensuring static variables are initialized.
    (void)register_swapper;
    (void)register_face_enhancer;
    (void)register_expression;
    (void)register_frame_enhancer;
}

} // namespace domain::pipeline

// Adapter Factory Methods Implementation
std::shared_ptr<IFrameProcessor> SwapperAdapter::create(const void* ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(ptr);
    if (!ctx) { throw std::runtime_error("Context is null"); }
    if (!ctx->swapper) { throw std::runtime_error("Swapper service not initialized in context"); }
    // Use actual model path if available, otherwise default (which will fail if loaded triggered)
    std::string path = ctx->swapper_model_path.empty() ? "default_model" : ctx->swapper_model_path;
    return std::shared_ptr<IFrameProcessor>(new SwapperAdapter(
        ctx->swapper, path, ctx->inference_options, ctx->occluder, ctx->region_masker));
}

std::shared_ptr<IFrameProcessor> FaceEnhancerAdapter::create(const void* ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(ptr);
    if (!ctx->face_enhancer) {
        throw std::runtime_error("Face enhancer service not initialized in context");
    }
    std::string path =
        ctx->enhancer_model_path.empty() ? "default_model" : ctx->enhancer_model_path;
    return std::shared_ptr<IFrameProcessor>(new FaceEnhancerAdapter(
        ctx->face_enhancer, path, ctx->inference_options, ctx->occluder, ctx->region_masker));
}

std::shared_ptr<IFrameProcessor> ExpressionAdapter::create(const void* ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(ptr);
    if (!ctx->restorer) {
        throw std::runtime_error("Expression restorer service not initialized in context");
    }
    std::string f_path =
        ctx->expression_feature_path.empty() ? "feat_path" : ctx->expression_feature_path;
    std::string m_path =
        ctx->expression_motion_path.empty() ? "motion_path" : ctx->expression_motion_path;
    std::string g_path =
        ctx->expression_generator_path.empty() ? "gen_path" : ctx->expression_generator_path;

    return std::shared_ptr<IFrameProcessor>(
        new ExpressionAdapter(ctx->restorer, f_path, m_path, g_path, ctx->inference_options,
                              ctx->occluder, ctx->region_masker));
}

std::shared_ptr<IFrameProcessor> FrameEnhancerAdapter::create(const void* ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(ptr);
    if (!ctx->frame_enhancer_factory) {
        throw std::runtime_error("Frame enhancer factory not provided in context");
    }
    return std::shared_ptr<IFrameProcessor>(new FrameEnhancerAdapter(ctx->frame_enhancer_factory));
}
