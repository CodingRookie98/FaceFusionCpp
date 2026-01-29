module;
#include <memory>
#include <string>

module domain.pipeline;

// Circular dependency causes build fail.
// Disabling registration logic temporarily as per "delete compilation error code" instruction.
// import services.pipeline.context;
// import processor_factory;

// using namespace services::pipeline;

// Factory registration helpers
namespace {
// const PipelineContext* cast_context(const void* ctx) {
//     return static_cast<const PipelineContext*>(ctx);
// }
}

/*
// Registration
static domain::pipeline::ProcessorRegistrar register_swapper("face_swapper", [](const void* ctx) {
    const auto* context = cast_context(ctx);
    return SwapperAdapter::create(context);
});

static domain::pipeline::ProcessorRegistrar register_face_enhancer("face_enhancer", [](const void*
ctx) { const auto* context = cast_context(ctx); return FaceEnhancerAdapter::create(context);
});

static domain::pipeline::ProcessorRegistrar register_expression("expression_restorer", [](const
void* ctx) { const auto* context = cast_context(ctx); return ExpressionAdapter::create(context);
});

static domain::pipeline::ProcessorRegistrar register_frame_enhancer("frame_enhancer", [](const void*
ctx) { const auto* context = cast_context(ctx); return FrameEnhancerAdapter::create(context);
});

// Adapter Factory Methods Implementation
std::shared_ptr<IFrameProcessor> SwapperAdapter::create(const void* context_ptr) {
    const auto* ctx = cast_context(context_ptr);
    if (!ctx->swapper) {
        throw std::runtime_error("Swapper service not initialized in context");
    }
    return std::make_shared<SwapperAdapter>(
        ctx->swapper,
        "default_model",
        ctx->inference_options,
        ctx->occluder,
        ctx->region_masker
    );
}

std::shared_ptr<IFrameProcessor> FaceEnhancerAdapter::create(const void* context_ptr) {
    const auto* ctx = cast_context(context_ptr);
    if (!ctx->face_enhancer) {
        throw std::runtime_error("Face enhancer service not initialized in context");
    }
    return std::make_shared<FaceEnhancerAdapter>(
        ctx->face_enhancer,
        "default_model",
        ctx->inference_options,
        ctx->occluder,
        ctx->region_masker
    );
}

std::shared_ptr<IFrameProcessor> ExpressionAdapter::create(const void* context_ptr) {
    const auto* ctx = cast_context(context_ptr);
    if (!ctx->restorer) {
        throw std::runtime_error("Expression restorer service not initialized in context");
    }
    return std::make_shared<ExpressionAdapter>(
        ctx->restorer,
        "feat_path", "motion_path", "gen_path",
        ctx->inference_options
    );
}

std::shared_ptr<IFrameProcessor> FrameEnhancerAdapter::create(const void* context_ptr) {
    const auto* ctx = cast_context(context_ptr);
    if (!ctx->frame_enhancer_factory) {
        throw std::runtime_error("Frame enhancer factory not provided in context");
    }
    return std::make_shared<FrameEnhancerAdapter>(ctx->frame_enhancer_factory);
}
*/