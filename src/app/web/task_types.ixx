/**
 * @file task_types.ixx
 * @brief Web task data structures (REST/WS API surface)
 */
module;

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <chrono>

export module app.web.task_types;

import config.task;

export namespace app::web {

/// Web task lifecycle status
enum class TaskStatus : std::uint8_t { Queued, Running, Done, Failed, Cancelled };

/// Progress snapshot exposed to API/WS clients
struct TaskProgress {
    std::size_t current_frame = 0;
    std::size_t total_frames = 0;
    double fps = 0.0;
};

/// Full task entry held by TaskManager
struct TaskEntry {
    std::string id;
    TaskStatus status = TaskStatus::Queued;
    config::TaskConfig config;
    TaskProgress progress;
    std::string error_message;
    std::vector<std::string> result_files; ///< Result file names inside output dir
    std::chrono::system_clock::time_point created_at;
    int priority = 0; ///< Higher value = higher scheduling priority
};

/// Lightweight task summary for list endpoints
struct TaskSummary {
    std::string id;
    TaskStatus status;
    TaskProgress progress;
    std::string error_message;
    std::size_t media_count = 0; ///< Number of target media
    std::chrono::system_clock::time_point created_at;
    int priority = 0;        ///< Higher value = higher scheduling priority
    int queue_position = -1; ///< 1-based position among queued tasks (-1 if not queued)
};

/// Bounding box of a face (coordinates relative to image pixels)
struct FaceBox {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

/// 2D landmark point
struct FacePoint {
    float x = 0.0F;
    float y = 0.0F;
};

/// Detected face info for API response
struct DetectedFaceInfo {
    int index = 0;
    FaceBox box;
    float score = 0.0F;
    std::string gender;
    std::pair<int, int> age_range{0, 0};
    std::vector<FacePoint> kps;
};

/// Face detection result for an image
struct FaceDetectionResult {
    std::string image_path;
    std::vector<DetectedFaceInfo> faces;
};

} // namespace app::web
