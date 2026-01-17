module;
#include <opencv2/core.hpp>
#include <map>
#include <string>
#include <any>
#include <vector>

export module domain.pipeline:types;

export namespace domain::pipeline {

struct FrameData {
    long long sequence_id = 0; // 帧序号
    double timestamp_ms = 0.0; // 时间戳
    cv::Mat image;             // 当前帧图像

    // 扩展字段：用于在不同处理器间传递中间结果
    // 例如：Detector 产生的 FaceLandmarks 可以存入此处供 Swapper 使用
    std::map<std::string, std::any> metadata;

    // 状态标识
    bool is_end_of_stream = false;
};

struct PipelineConfig {
    int worker_thread_count = 4;
    size_t max_queue_size = 32;
    // 显存并发限制 (例如同时只能有 2 个 GPU 任务)
    int max_concurrent_gpu_tasks = 2;
};
} // namespace domain::pipeline
