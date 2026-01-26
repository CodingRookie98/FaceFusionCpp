module;
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <functional>

export module services.pipeline.runner:types;

import domain.pipeline;
import domain.ai.model_repository;
import domain.face.masker;
import domain.face.analyser;
import foundation.ai.inference_session;

export namespace services::pipeline {

/**
 * @brief 任务进度信息 (帧级别)
 */
struct TaskProgress {
    std::string task_id;      ///< 任务ID
    size_t current_frame;     ///< 当前处理的帧序号
    size_t total_frames;      ///< 总帧数 (0 表示未知)
    std::string current_step; ///< 当前执行的 step 名称
    double fps;               ///< 当前处理速度 (帧/秒)
};

/**
 * @brief 进度回调函数类型
 */
using ProgressCallback = std::function<void(const TaskProgress&)>;

struct ProcessorContext {
    std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    std::vector<float> source_embedding;
    std::shared_ptr<domain::face::masker::IFaceOccluder> occluder;
    std::shared_ptr<domain::face::masker::IFaceRegionMasker> region_masker;
    std::shared_ptr<domain::face::analyser::FaceAnalyser> face_analyser;
    foundation::ai::inference_session::Options inference_options;
};

} // namespace services::pipeline
