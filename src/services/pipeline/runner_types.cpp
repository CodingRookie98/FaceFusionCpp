/**
 * @file runner_types.cpp
 * @brief Common types for the pipeline runner service
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <functional>
#include <optional>
#include <map>
#include <any>
#include <opencv2/core/mat.hpp>

export module services.pipeline.runner:types;

import domain.pipeline; // Imports domain::pipeline::FrameData
import domain.ai.model_repository;
import domain.face.masker;
import domain.face.analyser;
import foundation.ai.inference_session;
import services.pipeline.metrics;

export namespace services::pipeline {

using FrameData = domain::pipeline::FrameData; // Use domain type directly

/**
 * @brief Task progress information (frame level)
 */
struct TaskProgress {
    std::string task_id;      ///< Task identifier
    size_t current_frame;     ///< Current frame number being processed
    size_t total_frames;      ///< Total number of frames (0 if unknown)
    std::string current_step; ///< Name of the currently executing step
    double fps;               ///< Current processing speed (frames/sec)
};

/**
 * @brief Callback function type for reporting progress
 */
using ProgressCallback = std::function<void(const TaskProgress&)>;

/**
 * @brief Context object shared between pipeline processors
 */
struct ProcessorContext {
    std::shared_ptr<domain::ai::model_repository::ModelRepository>
        model_repo;                      ///< Repository for AI models
    std::vector<float> source_embedding; ///< Face embedding of the source face
    std::shared_ptr<domain::face::masker::IFaceOccluder>
        occluder; ///< Service for occlusion detection
    std::shared_ptr<domain::face::masker::IFaceRegionMasker>
        region_masker; ///< Service for face parsing
    std::shared_ptr<domain::face::analyser::FaceAnalyser>
        face_analyser; ///< Service for face analysis
    foundation::ai::inference_session::Options
        inference_options;                         ///< Configuration for ONNX inference
    MetricsCollector* metrics_collector = nullptr; ///< Performance metrics collector
};

} // namespace services::pipeline
