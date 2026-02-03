/**
 * @file metrics_collector.cpp
 * @brief MetricsCollector implementation
 */
module;

#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <thread>
#include <format>
#include <nlohmann/json.hpp>

module services.pipeline.metrics;

import foundation.infrastructure.logger;

namespace services::pipeline {

using json = nlohmann::json;
using foundation::infrastructure::logger::Logger;

MetricsCollector::MetricsCollector(const std::string& task_id) :
    m_task_id(task_id), m_start_time(std::chrono::steady_clock::now()),
    m_last_gpu_sample(m_start_time) {}

void MetricsCollector::set_total_frames(int64_t total) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_summary.total_frames = total;
}

void MetricsCollector::set_gpu_sample_interval(std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gpu_sample_interval = interval;
}

void MetricsCollector::start_step(const std::string& step_name) {
    auto tid = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_step_starts[step_name][tid] = std::chrono::steady_clock::now();
}

void MetricsCollector::end_step(const std::string& step_name) {
    auto end_time = std::chrono::steady_clock::now();
    auto tid = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it_step = m_step_starts.find(step_name);
    if (it_step == m_step_starts.end()) {
        Logger::get_instance()->warn(std::format(
            "[MetricsCollector] end_step called for unknown step: {}", step_name));
        return;
    }

    auto it_tid = it_step->second.find(tid);
    if (it_tid == it_step->second.end()) {
        std::ostringstream ss;
        ss << tid;
        Logger::get_instance()->warn(std::format(
            "[MetricsCollector] end_step called without matching start_step for step: {} in thread: {}", 
            step_name, ss.str()));
        return;
    }

    double duration_ms = std::chrono::duration<double, std::milli>(end_time - it_tid->second).count();

    m_step_samples[step_name].push_back(duration_ms);
    it_step->second.erase(it_tid);
}

void MetricsCollector::record_frame_completed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_summary.processed_frames++;
}

void MetricsCollector::record_frame_failed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_summary.failed_frames++;
}

void MetricsCollector::record_frame_skipped() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_summary.skipped_frames++;
}

void MetricsCollector::record_gpu_memory(int64_t usage_mb) {
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_mutex);

    // Rate limit sampling
    if (now - m_last_gpu_sample < m_gpu_sample_interval) { return; }
    m_last_gpu_sample = now;

    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time).count();

    m_gpu_samples.push_back({elapsed, usage_mb});
    m_gpu_peak_mb = std::max(m_gpu_peak_mb, usage_mb);
    m_gpu_sum_mb += usage_mb;
    m_gpu_sample_count++;
}

std::string MetricsCollector::to_json() const {
    auto metrics = get_metrics();

    json j;
    j["schema_version"] = metrics.schema_version;
    j["task_id"] = metrics.task_id;
    j["timestamp"] = metrics.timestamp;
    j["duration_ms"] = metrics.duration_ms;

    // Summary
    j["summary"] = {{"total_frames", metrics.summary.total_frames},
                    {"processed_frames", metrics.summary.processed_frames},
                    {"failed_frames", metrics.summary.failed_frames},
                    {"skipped_frames", metrics.summary.skipped_frames}};

    // Step latency
    j["step_latency"] = json::array();
    for (const auto& latency : metrics.step_latency) {
        j["step_latency"].push_back({{"step_name", latency.step_name},
                                     {"avg_ms", latency.avg_ms},
                                     {"p50_ms", latency.p50_ms},
                                     {"p99_ms", latency.p99_ms},
                                     {"total_ms", latency.total_ms},
                                     {"sample_count", latency.sample_count}});
    }

    // GPU memory
    j["gpu_memory"] = {{"peak_mb", metrics.gpu_memory.peak_mb},
                       {"avg_mb", metrics.gpu_memory.avg_mb},
                       {"samples", json::array()}};

    for (const auto& sample : metrics.gpu_memory.samples) {
        j["gpu_memory"]["samples"].push_back(
            {{"timestamp_ms", sample.timestamp_ms}, {"usage_mb", sample.usage_mb}});
    }

    return j.dump(2); // Pretty print
}

TaskMetrics MetricsCollector::get_metrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    TaskMetrics m;
    m.task_id = m_task_id;
    m.timestamp = get_iso_timestamp();

    auto now = std::chrono::steady_clock::now();
    m.duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time).count();

    m.summary = m_summary;

    // Calculate step latencies
    for (const auto& [name, samples] : m_step_samples) {
        m.step_latency.push_back(calculate_step_latency(name, samples));
    }

    // GPU Stats
    m.gpu_memory.peak_mb = m_gpu_peak_mb;
    m.gpu_memory.avg_mb = m_gpu_sample_count > 0 ? m_gpu_sum_mb / m_gpu_sample_count : 0;
    m.gpu_memory.samples = m_gpu_samples;

    return m;
}

bool MetricsCollector::export_json(const std::filesystem::path& output_path) {
    std::string path_str = output_path.string();
    path_str = replace_timestamp_placeholder(path_str);

    std::filesystem::path final_path(path_str);

    try {
        // Ensure directory exists
        auto parent = final_path.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent);
        }

        std::ofstream ofs(final_path);
        if (!ofs) {
            Logger::get_instance()->error(
                std::format("[MetricsCollector] Failed to create metrics file: {}", path_str));
            return false;
        }

        ofs << to_json();
        ofs.close();

        Logger::get_instance()->info(
            std::format("[MetricsCollector] Exported metrics to: {}", path_str));
        return true;
    } catch (const std::exception& e) {
        Logger::get_instance()->error(std::format(
            "[MetricsCollector] Error exporting metrics to {}: {}", path_str, e.what()));
        return false;
    }
}

StepLatency MetricsCollector::calculate_step_latency(const std::string& name,
                                                     const std::vector<double>& samples) const {
    StepLatency result;
    result.step_name = name;
    result.sample_count = static_cast<int64_t>(samples.size());

    if (samples.empty()) { return result; }

    // Total
    result.total_ms = std::accumulate(samples.begin(), samples.end(), 0.0);

    // Average
    result.avg_ms = result.total_ms / samples.size();

    // Percentiles (need sorted copy)
    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    result.p50_ms = calculate_percentile(sorted, 0.50);
    result.p99_ms = calculate_percentile(sorted, 0.99);

    return result;
}

double MetricsCollector::calculate_percentile(std::vector<double> sorted_samples,
                                              double percentile) {
    if (sorted_samples.empty()) return 0.0;
    if (sorted_samples.size() == 1) return sorted_samples[0];

    double index = percentile * (sorted_samples.size() - 1);
    size_t lower = static_cast<size_t>(index);
    size_t upper = lower + 1;

    if (upper >= sorted_samples.size()) { return sorted_samples.back(); }

    double fraction = index - lower;
    return sorted_samples[lower] * (1.0 - fraction) + sorted_samples[upper] * fraction;
}

std::string MetricsCollector::get_iso_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string MetricsCollector::replace_timestamp_placeholder(const std::string& path) const {
    std::string ts = get_iso_timestamp();
    // Replace characters that are invalid in filenames
    std::replace(ts.begin(), ts.end(), ':', '-');

    std::string result = path;
    size_t pos = result.find("{timestamp}");
    if (pos != std::string::npos) { result.replace(pos, 11, ts); }
    return result;
}

} // namespace services::pipeline
