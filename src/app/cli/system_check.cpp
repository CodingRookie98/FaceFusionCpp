module;

#include <format>
#include <nlohmann/json.hpp>

// CUDA/TensorRT headers (conditional)
#ifdef CUDA_ENABLED
#include <cuda_runtime.h>
#endif

module app.cli.system_check;

import foundation.media.ffmpeg;
import foundation.ai.inference_session;

namespace app::cli {

using json = nlohmann::json;

SystemCheckReport run_all_checks() {
    SystemCheckReport report;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. CUDA Driver Check
    // ─────────────────────────────────────────────────────────────────────────
#ifdef CUDA_ENABLED
    int cuda_version = 0;
    cudaError_t err = cudaDriverGetVersion(&cuda_version);
    if (err == cudaSuccess && cuda_version > 0) {
        int major = cuda_version / 1000;
        int minor = (cuda_version % 1000) / 10;
        report.checks.push_back(
            {"cuda_driver", CheckStatus::Ok, std::format("{}.{}", major, minor), ""});
    } else {
        report.checks.push_back(
            {"cuda_driver", CheckStatus::Fail, "Not Found", "CUDA driver not available"});
    }

    // VRAM Check
    size_t free_mem, total_mem;
    if (cudaMemGetInfo(&free_mem, &total_mem) == cudaSuccess) {
        double free_gb = free_mem / (1024.0 * 1024.0 * 1024.0);
        auto status = (free_gb >= 8.0) ? CheckStatus::Ok : CheckStatus::Warn;
        std::string msg = (free_gb < 8.0) ? "Recommended: 8GB+" : "";
        report.checks.push_back({"vram", status, std::format("{:.1f}GB", free_gb), msg});
    }
#else
    report.checks.push_back(
        {"cuda_driver", CheckStatus::Warn, "N/A", "Built without CUDA support"});
#endif

    // ─────────────────────────────────────────────────────────────────────────
    // 2. FFmpeg Libraries
    // ─────────────────────────────────────────────────────────────────────────
    auto ffmpeg_version = foundation::media::ffmpeg::get_version_string();
    report.checks.push_back({"ffmpeg_libs", CheckStatus::Ok, ffmpeg_version, ""});

    // ─────────────────────────────────────────────────────────────────────────
    // 3. ONNX Runtime
    // ─────────────────────────────────────────────────────────────────────────
    auto ort_info = foundation::ai::inference_session::get_runtime_info();
    report.checks.push_back({"onnxruntime", CheckStatus::Ok,
                             std::format("{} ({})", ort_info.version, ort_info.provider), ""});

    // ─────────────────────────────────────────────────────────────────────────
    // 4. Model Repository
    // ─────────────────────────────────────────────────────────────────────────
    // TODO: Count models in assets/models/

    // Calculate summary
    for (const auto& c : report.checks) {
        switch (c.status) {
        case CheckStatus::Ok: report.ok_count++; break;
        case CheckStatus::Warn: report.warn_count++; break;
        case CheckStatus::Fail: report.fail_count++; break;
        }
    }

    return report;
}

std::string format_text(const SystemCheckReport& report) {
    std::string result;
    for (const auto& c : report.checks) {
        std::string prefix;
        switch (c.status) {
        case CheckStatus::Ok: prefix = "[OK]"; break;
        case CheckStatus::Warn: prefix = "[WARN]"; break;
        case CheckStatus::Fail: prefix = "[FAIL]"; break;
        }
        result += std::format("{} {}: {}", prefix, c.name, c.value);
        if (!c.message.empty()) { result += " (" + c.message + ")"; }
        result += "\n";
    }
    result += "---\n";
    result += std::format("Result: {} FAIL, {} WARN\n", report.fail_count, report.warn_count);
    return result;
}

std::string format_json(const SystemCheckReport& report) {
    json j;
    j["checks"] = json::array();
    for (const auto& c : report.checks) {
        json item;
        item["name"] = c.name;
        item["status"] = (c.status == CheckStatus::Ok)   ? "ok" :
                         (c.status == CheckStatus::Warn) ? "warn" :
                                                           "fail";
        item["value"] = c.value;
        if (!c.message.empty()) { item["message"] = c.message; }
        j["checks"].push_back(item);
    }
    j["summary"]["ok"] = report.ok_count;
    j["summary"]["warn"] = report.warn_count;
    j["summary"]["fail"] = report.fail_count;
    return j.dump(2);
}

} // namespace app::cli
