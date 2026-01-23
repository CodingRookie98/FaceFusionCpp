/**
 * @file task_config.ixx
 * @brief 任务配置结构体定义 (对应 task_config.yaml)
 *
 * 定义任务级配置结构，包括：
 * - 任务元信息
 * - 输入输出配置
 * - 资源控制配置
 * - Pipeline 步骤定义
 */
module;

// Global Module Fragment - 标准库头文件必须在此处 #include
#include <string>
#include <vector>
#include <optional>
#include <variant>

export module config.task;

export import config.types;

export namespace config {

/**
 * @brief 任务元信息
 */
struct TaskInfo {
    std::string id;             ///< 唯一任务标识 (格式: [a-zA-Z0-9_])
    std::string description;    ///< 任务描述
    bool enable_logging = true; ///< 是否启用独立任务日志
    bool enable_resume = false; ///< 是否启用断点续处理
};

/**
 * @brief 输出配置
 */
struct OutputConfig {
    std::string path;                      ///< 输出路径 (强制绝对路径)
    std::string prefix = "result_";        ///< 输出文件前缀
    std::string suffix;                    ///< 输出文件后缀
    std::string image_format = "png";      ///< 图片格式 [png, jpg, bmp]
    std::string video_encoder = "libx264"; ///< 视频编码器
    int video_quality = 80;                ///< 视频质量 [0-100]
    ConflictPolicy conflict_policy = ConflictPolicy::Error;
    AudioPolicy audio_policy = AudioPolicy::Copy;
};

/**
 * @brief IO 配置
 */
struct IOConfig {
    std::vector<std::string> source_paths; ///< 源路径列表 (仅支持图片，可含目录)
    std::vector<std::string> target_paths; ///< 目标路径列表 (图片/视频/目录混合)
    OutputConfig output;
};

/**
 * @brief 任务资源控制配置
 */
struct TaskResourceConfig {
    int thread_count = 0; ///< 线程数，0 = 自动 (50% CPU)
    ExecutionOrder execution_order = ExecutionOrder::Sequential;
    MemoryStrategy memory_strategy = MemoryStrategy::Tolerant; ///< 内存策略
    int segment_duration_seconds = 0;                          ///< 视频分段秒数，0 = 不分段
};

// ============================================================================
// Pipeline Step 参数定义
// ============================================================================

/**
 * @brief Face Swapper 参数
 */
struct FaceSwapperParams {
    std::string model; ///< 模型名称 [inswapper_128, inswapper_128_fp16]
    FaceSelectorMode face_selector_mode = FaceSelectorMode::Many;
    std::optional<std::string> reference_face_path; ///< 参考人脸路径 (mode=reference 时必需)
};

/**
 * @brief Face Enhancer 参数
 */
struct FaceEnhancerParams {
    std::string model;         ///< 模型名称 [codeformer, gfpgan_1.2, gfpgan_1.3, gfpgan_1.4]
    double blend_factor = 0.8; ///< 融合因子 [0.0-1.0]
    FaceSelectorMode face_selector_mode = FaceSelectorMode::Many;
    std::optional<std::string> reference_face_path;
};

/**
 * @brief Expression Restorer 参数
 */
struct ExpressionRestorerParams {
    std::string model;           ///< 模型名称 [live_portrait]
    double restore_factor = 0.8; ///< 恢复因子 [0.0-1.0]
    FaceSelectorMode face_selector_mode = FaceSelectorMode::Many;
    std::optional<std::string> reference_face_path;
};

/**
 * @brief Frame Enhancer 参数
 */
struct FrameEnhancerParams {
    std::string model;           ///< 模型名称 [real_esrgan_x2, real_esrgan_x4, ...]
    double enhance_factor = 0.8; ///< 增强因子 [0.0-1.0]
};

/**
 * @brief Step 参数变体类型
 */
using StepParams = std::variant<FaceSwapperParams, FaceEnhancerParams, ExpressionRestorerParams,
                                FrameEnhancerParams>;

/**
 * @brief Pipeline 步骤定义
 *
 * 支持多个同类型 Step，每个 Step 的 name 和 params 可不同。
 */
struct PipelineStep {
    std::string step;    ///< 处理器类型 (face_swapper, face_enhancer, ...)
    std::string name;    ///< 步骤别名
    bool enabled = true; ///< 是否启用
    StepParams params;   ///< 步骤参数
};

// ============================================================================
// Face Analysis 配置
// ============================================================================

/**
 * @brief 人脸检测器配置
 */
struct FaceDetectorConfig {
    std::vector<std::string> models = {"yoloface", "retinaface", "scrfd"};
    double score_threshold = 0.5;
};

/**
 * @brief 人脸关键点检测器配置
 */
struct FaceLandmarkerConfig {
    std::string model = "2dfan4"; ///< [2dfan4, peppa_wutz, face_landmarker_68_5]
};

/**
 * @brief 人脸识别器配置
 */
struct FaceRecognizerConfig {
    std::string model = "arcface_w600k_r50";
    double similarity_threshold = 0.6; ///< 相似度阈值 [0.0-1.0]
};

/**
 * @brief 人脸遮罩配置
 */
struct FaceMaskerConfig {
    std::vector<std::string> types = {"box", "occlusion", "region"};
    std::vector<std::string> region = {"face", "eyes"};
};

/**
 * @brief 人脸分析配置
 */
struct FaceAnalysisConfig {
    FaceDetectorConfig face_detector;
    FaceLandmarkerConfig face_landmarker;
    FaceRecognizerConfig face_recognizer;
    FaceMaskerConfig face_masker;
};

// ============================================================================
// 完整 TaskConfig
// ============================================================================

/**
 * @brief 任务配置 (对应 task_config.yaml)
 *
 * 动态配置，每次任务执行时加载。
 */
struct TaskConfig {
    std::string config_version; ///< 配置版本，必须为 "1.0"
    TaskInfo task_info;
    IOConfig io;
    TaskResourceConfig resource;
    FaceAnalysisConfig face_analysis;
    std::vector<PipelineStep> pipeline;
};

} // namespace config
