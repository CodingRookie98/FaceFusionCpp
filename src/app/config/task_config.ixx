/**
 * @file task_config.ixx
 * @brief Task-specific configuration structures
 * @author CodingRookie
 * @date 2026-01-27
 * @details Defines structures corresponding to task_config.yaml, including
 *          task metadata, IO, resources, and pipeline step definitions.
 */
module;

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <thread>
#include <algorithm>

export module config.task;

export import config.types;

export namespace config {

/**
 * @brief Metadata for a specific processing task
 */
struct TaskInfo {
    std::string id;             ///< Unique task identifier (regex: [a-zA-Z0-9_])
    std::string description;    ///< Human-readable description
    bool enable_logging = true; ///< Whether to enable task-specific logging
    bool enable_resume = false; ///< Whether to support resuming from checkpoints
};

/**
 * @brief Output settings for the processed media
 */
struct OutputConfig {
    std::string path;                      ///< Target directory (absolute path)
    std::string prefix = "result_";        ///< Prefix for result filenames
    std::string suffix;                    ///< Suffix for result filenames
    std::string image_format = "png";      ///< Format for image results (png, jpg, bmp)
    std::string video_encoder = "libx264"; ///< FFmpeg video encoder name
    int video_quality = 80;                ///< Video encoding quality (0-100)
    ConflictPolicy conflict_policy = ConflictPolicy::Error; ///< Policy for existing files
    AudioPolicy audio_policy = AudioPolicy::Copy;           ///< Policy for audio track
};

/**
 * @brief IO configuration for input and output
 */
struct IOConfig {
    std::vector<std::string> source_paths; ///< Paths to source images (face providers)
    std::vector<std::string> target_paths; ///< Paths to target images or videos
    OutputConfig output;                   ///< Output specific settings
};

/**
 * @brief Resource management for a specific task
 */
struct TaskResourceConfig {
    int thread_count = 0;    ///< CPU threads (0 = auto)
    int max_queue_size = 20; ///< Max frames buffered in pipeline (default: 20)
    ExecutionOrder execution_order = ExecutionOrder::Sequential; ///< Media processing order
    MemoryStrategy memory_strategy = MemoryStrategy::Tolerant;   ///< Memory usage priority
    int segment_duration_seconds = 0;                            ///< Video segmenting (0 = off)
    int max_frames = 0; ///< Max frames to process (0 = all)

    /**
     * @brief Get the effective thread count (handling auto: half of hardware threads)
     */
    [[nodiscard]] int get_effective_thread_count() const {
        if (thread_count > 0) return thread_count;
        unsigned int hw = std::thread::hardware_concurrency();
        if (hw == 0) return 2; // Fallback
        return std::max(1, static_cast<int>(hw / 2));
    }
};

/**
 * @brief Parameters for Face Swapper processor
 */
struct FaceSwapperParams {
    std::string model;                                            ///< Model name identifier
    FaceSelectorMode face_selector_mode = FaceSelectorMode::Many; ///< Selection strategy
    std::optional<std::string> reference_face_path;               ///< Required for reference mode
};

/**
 * @brief Parameters for Face Enhancer processor
 */
struct FaceEnhancerParams {
    std::string model;         ///< Model name identifier
    double blend_factor = 0.8; ///< Face blending factor (0.0-1.0)
    FaceSelectorMode face_selector_mode = FaceSelectorMode::Many; ///< Selection strategy
    std::optional<std::string> reference_face_path;
};

/**
 * @brief Parameters for Expression Restorer processor
 */
struct ExpressionRestorerParams {
    std::string model;                                            ///< Model name identifier
    double restore_factor = 0.8;                                  ///< Restoration factor (0.0-1.0)
    FaceSelectorMode face_selector_mode = FaceSelectorMode::Many; ///< Selection strategy
    std::optional<std::string> reference_face_path;
};

/**
 * @brief Parameters for Frame Enhancer processor
 */
struct FrameEnhancerParams {
    std::string model;           ///< Model name identifier
    double enhance_factor = 0.8; ///< Enhancement intensity (0.0-1.0)
};

/**
 * @brief Variant type representing parameters for any pipeline step
 */
using StepParams = std::variant<FaceSwapperParams, FaceEnhancerParams, ExpressionRestorerParams,
                                FrameEnhancerParams>;

/**
 * @brief Definition of a single step in the processing pipeline
 */
struct PipelineStep {
    std::string step;    ///< Processor type (e.g., "face_swapper")
    std::string name;    ///< User-defined name for this step instance
    bool enabled = true; ///< Whether this step is active
    StepParams params;   ///< Processor-specific parameters
};

/**
 * @brief Configuration for face detection service
 */
struct FaceDetectorConfig {
    std::vector<std::string> models = {"yoloface", "retinaface", "scrfd"}; ///< Detector models
    double score_threshold = 0.5;                                          ///< Min confidence
};

/**
 * @brief Configuration for face landmark detection service
 */
struct FaceLandmarkerConfig {
    std::string model = "2dfan4"; ///< Preferred landmarker model
};

/**
 * @brief Configuration for face recognition service
 */
struct FaceRecognizerConfig {
    std::string model = "arcface_w600k_r50"; ///< Preferred recognizer model
    double similarity_threshold = 0.6;       ///< Face matching threshold
};

/**
 * @brief Configuration for face masking service
 */
struct FaceMaskerConfig {
    std::vector<std::string> types = {"box", "occlusion", "region"}; ///< Active maskers
    std::vector<std::string> region = {"face", "eyes"};              ///< Regions for parsing
};

/**
 * @brief Aggregate configuration for all face analysis components
 */
struct FaceAnalysisConfig {
    FaceDetectorConfig face_detector;
    FaceLandmarkerConfig face_landmarker;
    FaceRecognizerConfig face_recognizer;
    FaceMaskerConfig face_masker;
};

/**
 * @brief Root task configuration structure
 */
struct TaskConfig {
    std::string config_version;         ///< Config format version (e.g., "1.0")
    TaskInfo task_info;                 ///< Task metadata
    IOConfig io;                        ///< IO settings
    TaskResourceConfig resource;        ///< Resource settings
    FaceAnalysisConfig face_analysis;   ///< Face analysis settings
    std::vector<PipelineStep> pipeline; ///< Ordered list of processing steps
};

} // namespace config
