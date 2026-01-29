export module services.pipeline.context;

import <memory>;
import <string>;
import config.types;
import config.app;
import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import domain.face.masker;
import domain.frame.enhancer;
import foundation.ai.inference_session;

namespace services::pipeline {

struct PipelineContext {
    const config::AppConfig& app_config;
    const config::PipelineStep& step_config;

    // Shared services
    std::shared_ptr<domain::face::swapper::IFaceSwapper> swapper;
    std::shared_ptr<domain::face::enhancer::IFaceEnhancer> face_enhancer;
    std::shared_ptr<domain::face::expression::IFaceExpressionRestorer> restorer;
    std::shared_ptr<domain::face::masker::IFaceOccluder> occluder;
    std::shared_ptr<domain::face::masker::IFaceRegionMasker> region_masker;

    // Frame enhancer factory (since it might need custom creation per step)
    std::function<std::shared_ptr<domain::frame::enhancer::IFrameEnhancer>()>
        frame_enhancer_factory;

    // Inference options
    foundation::ai::inference_session::Options inference_options;
};

} // namespace services::pipeline
