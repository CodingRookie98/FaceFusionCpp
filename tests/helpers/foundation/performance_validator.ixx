module;
#include <chrono>
#include <string>
#include <functional>
#include <stdexcept>

export module tests.helpers.foundation.performance_validator;

export namespace tests::helpers::foundation {

/**
 * @brief 硬件配置级别 (基于 GPU VRAM)
 */
enum class HardwareProfile {
    LOW_END,   // GTX 1650 (4GB)
    MID_RANGE, // RTX 4060 (8GB) / RTX 3060 (12GB)
    HIGH_END   // RTX 4090 (24GB)
};

/**
 * @brief 验收标准阈值
 */
struct AcceptanceCriteria {
    // 图片处理耗时阈值 (毫秒)
    int64_t img_512_max_ms = 1000;
    int64_t img_720p_max_ms = 2000;
    int64_t img_2k_max_ms = 3000;

    // 视频处理
    double video_min_fps = 15.0;
    int64_t video_max_duration_s = 40;

    // 资源限制
    int64_t max_vram_mb = 6500;
};

/**
 * @brief 获取硬件配置对应的验收标准
 */
inline AcceptanceCriteria get_criteria_for_profile(HardwareProfile profile) {
    AcceptanceCriteria criteria;

    switch (profile) {
    case HardwareProfile::LOW_END: // GTX 1650 (4GB)
        criteria.img_512_max_ms = 3000;
        criteria.img_720p_max_ms = 5000;
        criteria.img_2k_max_ms = 10000;
        criteria.video_min_fps = 5.0;
        criteria.video_max_duration_s = 120;
        criteria.max_vram_mb = 3500;
        break;

    case HardwareProfile::MID_RANGE: // RTX 4060 (8GB) / RTX 3060 (12GB)
        criteria.img_512_max_ms = 1000;
        criteria.img_720p_max_ms = 2000;
        criteria.img_2k_max_ms = 3000;
        criteria.video_min_fps = 15.0;
        criteria.video_max_duration_s = 40;
        criteria.max_vram_mb = 6500;
        break;

    case HardwareProfile::HIGH_END: // RTX 4090 (24GB)
        criteria.img_512_max_ms = 500;
        criteria.img_720p_max_ms = 1000;
        criteria.img_2k_max_ms = 2000;
        criteria.video_min_fps = 30.0;
        criteria.video_max_duration_s = 20;
        criteria.max_vram_mb = 18000;
        break;
    }

    return criteria;
}

/**
 * @brief 自动检测当前硬件配置
 */
inline HardwareProfile detect_hardware_profile() {
    // 通过 NVML 或系统 API 检测 GPU VRAM
    // 简化实现: 默认返回 LOW_END
    // TODO: 实现真实 GPU 检测
    return HardwareProfile::LOW_END;
}

/**
 * @brief 性能测试结果结构
 */
struct PerformanceResult {
    int64_t duration_ms;
    double fps; // 仅视频
    int64_t peak_vram_mb;
    bool passed;
    std::string failure_reason;
};

/**
 * @brief 执行带计时的操作并验证性能
 */
template <typename Func>
PerformanceResult measure_and_validate(Func&& operation, int64_t max_duration_ms,
                                       const std::string& test_name) {
    using namespace std::chrono;

    PerformanceResult result;

    auto start = steady_clock::now();
    try {
        operation();
    } catch (const std::exception& e) {
        result.passed = false;
        result.failure_reason = std::string("Exception: ") + e.what();
        return result;
    }

    result.duration_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();
    result.passed = (result.duration_ms <= max_duration_ms);

    if (!result.passed) {
        result.failure_reason = test_name
                              + " exceeded time limit: " + std::to_string(result.duration_ms)
                              + "ms > " + std::to_string(max_duration_ms) + "ms";
    }

    return result;
}

} // namespace tests::helpers::foundation
