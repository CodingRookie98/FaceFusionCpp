/**
 * @file metrics_collector.ixx
 * @brief Performance metrics collection and JSON export
 * @author CodingRookie
 * @date 2026-01-31
 * @see design.md Section 5.11 - Metrics JSON Schema
 */
module;

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <thread>
#include <cstdint>

export module services.pipeline.metrics;

export namespace services::pipeline {

// ─────────────────────────────────────────────────────────────────────────────
// Data Structures (matching JSON schema)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Latency statistics for a single pipeline step
 */
struct StepLatency {
    std::string step_name;
    double avg_ms = 0.0;
    double p50_ms = 0.0; ///< Median
    double p99_ms = 0.0; ///< 99th percentile
    double total_ms = 0.0;
    int64_t sample_count = 0;
};

/**
 * @brief GPU memory sample at a point in time
 */
struct GpuMemorySample {
    int64_t timestamp_ms;
    int64_t usage_mb;
};

/**
 * @brief Processing summary statistics
 */
struct ProcessingSummary {
    int64_t total_frames = 0;
    int64_t processed_frames = 0;
    int64_t failed_frames = 0;
    int64_t skipped_frames = 0; ///< Frames skipped due to no face detected
};

/**
 * @brief GPU memory statistics
 */
struct GpuMemoryStats {
    int64_t peak_mb = 0;
    int64_t avg_mb = 0;
    std::vector<GpuMemorySample> samples;
};

/**
 * @brief Complete task metrics structure
 */
struct TaskMetrics {
    std::string schema_version = "1.0";
    std::string task_id;
    std::string timestamp; ///< ISO 8601 format
    int64_t duration_ms = 0;

    ProcessingSummary summary;
    std::vector<StepLatency> step_latency;
    GpuMemoryStats gpu_memory;
};

// ─────────────────────────────────────────────────────────────────────────────
// MetricsCollector Class
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Collects and exports performance metrics during processing
 * @details Thread-safe metrics collector that tracks:
 *          - Per-step latency (with percentile calculation)
 *          - GPU memory usage over time
 *          - Frame processing summary
 */
class MetricsCollector {
public:
    /**
     * @brief Construct metrics collector
     * @param task_id Task identifier for this collection
     */
    explicit MetricsCollector(const std::string& task_id);

    ~MetricsCollector() = default;

    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    MetricsCollector(MetricsCollector&&) = delete;
    MetricsCollector& operator=(MetricsCollector&&) = delete;

    // ─────────────────────────────────────────────────────────────────────────
    // Configuration
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Set total expected frames
     */
    void set_total_frames(int64_t total);

    /**
     * @brief Set GPU memory sampling interval
     */
    void set_gpu_sample_interval(std::chrono::milliseconds interval);

    // ─────────────────────────────────────────────────────────────────────────
    // Step Timing
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Start timing a processing step
     * @param step_name Name of the step (e.g., "face_swap", "enhance")
     */
    void start_step(const std::string& step_name);

    /**
     * @brief End timing for a step and record the sample
     * @param step_name Must match a previous start_step call
     */
    void end_step(const std::string& step_name);

    // ─────────────────────────────────────────────────────────────────────────
    // Frame Tracking
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Record successful frame processing
     */
    void record_frame_completed();

    /**
     * @brief Record frame processing failure
     */
    void record_frame_failed();

    /**
     * @brief Record frame skipped (e.g., no face detected)
     */
    void record_frame_skipped();

    // ─────────────────────────────────────────────────────────────────────────
    // GPU Memory Tracking
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Record current GPU memory usage
     * @param usage_mb Memory usage in megabytes
     */
    void record_gpu_memory(int64_t usage_mb);

    // ─────────────────────────────────────────────────────────────────────────
    // Export
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Export metrics to JSON file
     * @param output_path Path for output file (supports {timestamp} placeholder)
     * @return true if export succeeded
     */
    bool export_json(const std::filesystem::path& output_path);

    /**
     * @brief Export metrics to JSON string
     */
    [[nodiscard]] std::string to_json() const;

    /**
     * @brief Get current metrics snapshot
     */
    [[nodiscard]] TaskMetrics get_metrics() const;

private:
    mutable std::mutex m_mutex;

    std::string m_task_id;
    std::chrono::steady_clock::time_point m_start_time;

    // Step timing
    std::unordered_map<std::string,
                       std::unordered_map<std::thread::id, std::chrono::steady_clock::time_point>>
        m_step_starts;
    std::unordered_map<std::string, std::vector<double>> m_step_samples;

    // Frame counting
    ProcessingSummary m_summary;

    // GPU memory
    std::chrono::steady_clock::time_point m_last_gpu_sample;
    std::chrono::milliseconds m_gpu_sample_interval{1000};
    std::vector<GpuMemorySample> m_gpu_samples;
    int64_t m_gpu_peak_mb = 0;
    int64_t m_gpu_sum_mb = 0;
    int64_t m_gpu_sample_count = 0;

    // ─────────────────────────────────────────────────────────────────────────
    // Internal Helpers
    // ─────────────────────────────────────────────────────────────────────────

    [[nodiscard]] StepLatency calculate_step_latency(const std::string& name,
                                                     const std::vector<double>& samples) const;

    [[nodiscard]] static double calculate_percentile(std::vector<double> sorted_samples,
                                                     double percentile);

    [[nodiscard]] std::string get_iso_timestamp() const;
    [[nodiscard]] std::string replace_timestamp_placeholder(const std::string& path) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// RAII Helper
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RAII helper for automatic step timing
 */
class ScopedStepTimer {
public:
    ScopedStepTimer(MetricsCollector& collector, std::string step_name) :
        m_collector(collector), m_step_name(std::move(step_name)) {
        m_collector.start_step(m_step_name);
    }

    ~ScopedStepTimer() { m_collector.end_step(m_step_name); }

    ScopedStepTimer(const ScopedStepTimer&) = delete;
    ScopedStepTimer& operator=(const ScopedStepTimer&) = delete;
    ScopedStepTimer(ScopedStepTimer&&) = delete;
    ScopedStepTimer& operator=(ScopedStepTimer&&) = delete;

private:
    MetricsCollector& m_collector;
    std::string m_step_name;
};

} // namespace services::pipeline
