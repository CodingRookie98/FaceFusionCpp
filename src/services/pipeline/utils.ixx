module;
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

export module services.pipeline.utils;

export namespace services::pipeline {

using IsVideoCheck = std::function<bool(const std::string&)>;

struct SortedTargets {
    std::vector<std::string> images;
    std::vector<std::string> videos;
};

/**
 * @brief Sorts target paths into images and videos while preserving relative order
 * @param targets List of target paths
 * @param is_video Predicate to check if a path is a video
 * @return SortedTargets struct containing separated lists
 */
SortedTargets sort_targets_by_type(const std::vector<std::string>& targets, IsVideoCheck is_video) {
    SortedTargets result;
    // Reserve to avoid multiple reallocations
    result.images.reserve(targets.size());
    result.videos.reserve(targets.size());

    for (const auto& path : targets) {
        if (is_video(path)) {
            result.videos.push_back(path);
        } else {
            result.images.push_back(path);
        }
    }
    return result;
}

} // namespace services::pipeline
