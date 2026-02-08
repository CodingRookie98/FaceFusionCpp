/**
 * @file pipeline_types.ixx
 * @brief Common types for the processing pipeline
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/core.hpp>
#include <map>
#include <string>
#include <any>
#include <optional>
#include <vector>
#include <memory>

export module domain.pipeline:types;

import domain.face; // For domain::face::Face
import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;

export namespace domain::pipeline {

/**
 * @brief Container for a single frame and its associated metadata
 * @details This structure is passed through the pipeline processors.
 */
struct FrameData {
    std::int64_t sequence_id = 0; ///< Sequential frame number (0-based)
    double timestamp_ms = 0.0; ///< Presentation timestamp in milliseconds
    cv::Mat image;             ///< Current frame image data (BGR)

    // Optimized Metadata (Strongly Typed)
    // Avoid std::any for high-frequency data
    std::shared_ptr<const std::vector<float>> source_embedding; // From Runner (global context)
    std::vector<domain::face::Face> detected_faces;             // From Face Analysis Step

    // Processor Specific Inputs (Pre-calculated by Analysis Step)
    // Using std::optional to indicate presence without map lookup
    std::optional<domain::face::swapper::SwapInput> swap_input;
    std::optional<domain::face::enhancer::EnhanceInput> enhance_input;
    std::optional<domain::face::expression::RestoreExpressionInput> expression_input;

    /**
     * @brief Intermediate results shared between processors
     * @details Example: Key "landmarks" might contain face keypoints from a detector.
     */
    std::map<std::string, std::any> metadata;

    bool is_end_of_stream = false; ///< Signal to indicate end of media stream
};

/**
 * @brief Global configuration for the pipeline execution engine
 */
struct PipelineConfig {
    int worker_thread_count = 4;      ///< Number of CPU threads for processing
    size_t max_queue_size = 32;       ///< Max frames buffered in the pipeline
    int max_concurrent_gpu_tasks = 2; ///< Max simultaneous GPU inference tasks
};

} // namespace domain::pipeline
