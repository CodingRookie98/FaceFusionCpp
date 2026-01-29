module;
#include <memory>
#include <string>
#include "processor_factory.ixx"
#include "pipeline_context.ixx"

module domain.pipeline;

using namespace services::pipeline; // For PipelineContext

// ... (Implementations of Adapter constructors) ...

// Registration
static domain::pipeline::ProcessorRegistrar register_swapper("face_swapper", [](const void* ctx) {
    const auto* context = static_cast<const PipelineContext*>(ctx);
    // Logic to extract specific params from context->step_config if needed
    // For now, assume model path comes from config or default
    std::string model_path = "inswapper_128_fp16.onnx"; // Simplified
    return SwapperAdapter::create(context);
});

static domain::pipeline::ProcessorRegistrar register_face_enhancer(
    "face_enhancer", [](const void* ctx) {
        const auto* context = static_cast<const PipelineContext*>(ctx);
        return FaceEnhancerAdapter::create(context);
    });

static domain::pipeline::ProcessorRegistrar register_expression(
    "expression_restorer", [](const void* ctx) {
        const auto* context = static_cast<const PipelineContext*>(ctx);
        return ExpressionAdapter::create(context);
    });

static domain::pipeline::ProcessorRegistrar register_frame_enhancer(
    "frame_enhancer", [](const void* ctx) {
        const auto* context = static_cast<const PipelineContext*>(ctx);
        return FrameEnhancerAdapter::create(context);
    });

// Adapter Factory Methods Implementation
std::shared_ptr<IFrameProcessor> SwapperAdapter::create(const void* context_ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(context_ptr);
    // Extract specific params logic...
    return std::make_shared<SwapperAdapter>(ctx->swapper, "model_path", ctx->inference_options,
                                            ctx->occluder, ctx->region_masker);
}

std::shared_ptr<IFrameProcessor> FaceEnhancerAdapter::create(const void* context_ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(context_ptr);
    return std::make_shared<FaceEnhancerAdapter>(ctx->face_enhancer, "model_path",
                                                 ctx->inference_options, ctx->occluder,
                                                 ctx->region_masker);
}

std::shared_ptr<IFrameProcessor> ExpressionAdapter::create(const void* context_ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(context_ptr);
    return std::make_shared<ExpressionAdapter>(ctx->restorer, "feat", "motion", "gen",
                                               ctx->inference_options);
}

std::shared_ptr<IFrameProcessor> FrameEnhancerAdapter::create(const void* context_ptr) {
    const auto* ctx = static_cast<const PipelineContext*>(context_ptr);
    return std::make_shared<FrameEnhancerAdapter>(ctx->frame_enhancer_factory);
}
