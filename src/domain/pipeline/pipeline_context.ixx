module;

#include <memory>
#include <functional>
#include <string>

export module domain.pipeline.context;

import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import domain.face.masker;
import domain.frame.enhancer;
import foundation.ai.inference_session;

export namespace domain::pipeline {

struct PipelineContext {
    // config::AppConfig and config::PipelineStep removed due to compilation errors

    // Shared services
    std::shared_ptr<domain::face::swapper::IFaceSwapper> swapper;
    std::shared_ptr<domain::face::enhancer::IFaceEnhancer> face_enhancer;
    std::shared_ptr<domain::face::expression::IFaceExpressionRestorer> restorer;
    std::shared_ptr<domain::face::masker::IFaceOccluder> occluder;
    std::shared_ptr<domain::face::masker::IFaceRegionMasker> region_masker;

    // Frame enhancer factory
    std::function<std::shared_ptr<domain::frame::enhancer::IFrameEnhancer>()>
        frame_enhancer_factory;

    // Inference options
    foundation::ai::inference_session::Options inference_options;

    // Model paths (Optional, for adapters)
    std::string swapper_model_path;
    std::string enhancer_model_path;
    std::string expression_feature_path;
    std::string expression_motion_path;
    std::string expression_generator_path;
    std::string frame_enhancer_model_path;
};

} // namespace domain::pipeline