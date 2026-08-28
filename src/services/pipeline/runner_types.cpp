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
#include <algorithm>
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

/**
 * @brief Keyed cache for domain service instances
 * @details Reuses services (e.g. swapper/enhancer/restorer) across pipeline builds
 *          (multi-video tasks, segmented mode) so per-video ONNX initializer reparse
 *          and object reconstruction are avoided. Type-erased storage; callers
 *          static_pointer_cast to the concrete interface.
 */
class DomainServiceCache {
public:
    std::shared_ptr<void> get_or_create(const std::string& key,
                                        std::function<std::shared_ptr<void>()> factory) {
        const std::lock_guard kLock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) { return it->second; }
        auto instance = factory();
        if (instance) { m_cache.emplace(key, instance); }
        return instance;
    }

    [[nodiscard]] size_t size() const {
        const std::lock_guard kLock(m_mutex);
        return m_cache.size();
    }

    void clear() {
        const std::lock_guard kLock(m_mutex);
        m_cache.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<void>> m_cache;
};

inline constexpr int kStrictQueueCap = 16; ///< Strict 内存模式 Pipeline 帧队列上限

/**
 * @brief Strict 内存模式下 Pipeline 帧队列容量上限
 * @param configured 用户配置的 max_queue_size（<=0 表示未配置）
 * @return min(configured, kStrictQueueCap)，未配置时返回 kStrictQueueCap
 */
int strict_queue_limit(int configured) {
    if (configured <= 0) return kStrictQueueCap;
    return std::min(configured, kStrictQueueCap);
}

} // namespace services::pipeline
