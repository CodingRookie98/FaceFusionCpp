module;

#include <format>
#include <filesystem>
#include <nlohmann/json.hpp>

// CUDA/TensorRT headers (conditional)
#ifdef CUDA_ENABLED
#include <cuda_runtime.h>
#include <cudnn.h>
#endif

#ifdef TENSORRT_ENABLED
#include <NvInferVersion.h>
#endif

module app.cli.system_check;

import foundation.media.ffmpeg;
import foundation.ai.inference_session;

namespace app::cli {

using json = nlohmann::json;

SystemCheckReport run_all_checks() {
    SystemCheckReport report;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. CUDA Runtime Check
    // ─────────────────────────────────────────────────────────────────────────
#ifdef CUDA_ENABLED
    int cuda_version = 0;
    cudaError_t err = cudaRuntimeGetVersion(&cuda_version);
    if (err == cudaSuccess && cuda_version > 0) {
        int major = cuda_version / 1000;
        int minor = (cuda_version % 1000) / 10;
        report.checks.push_back(
            {"cuda_runtime", CheckStatus::Ok, std::format("{}.{}", major, minor), ""});

        // cuDNN Check
        size_t cudnn_version = cudnnGetVersion();
        // cuDNN 9+ uses major * 10000 + minor * 100 + patch
        // cuDNN 8 uses major * 1000 + minor * 100 + patch
        int cudnn_major, cudnn_minor, cudnn_patch;
        if (cudnn_version >= 90000) {
            cudnn_major = cudnn_version / 10000;
            cudnn_minor = (cudnn_version % 10000) / 100;
            cudnn_patch = cudnn_version % 100;
        } else {
            cudnn_major = cudnn_version / 1000;
            cudnn_minor = (cudnn_version % 1000) / 100;
            cudnn_patch = cudnn_version % 100;
        }
        report.checks.push_back({"cudnn", CheckStatus::Ok,
                                 std::format("{}.{}.{}", cudnn_major, cudnn_minor, cudnn_patch),
                                 ""});
    } else {
        report.checks.push_back(
            {"cuda_runtime", CheckStatus::Fail, "Not Found", "CUDA runtime not available"});
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

#ifdef TENSORRT_ENABLED
    // 1.2 TensorRT Check
    report.checks.push_back(
        {"tensorrt", CheckStatus::Ok,
         std::format("{}.{}.{}", NV_TENSORRT_MAJOR, NV_TENSORRT_MINOR, NV_TENSORRT_PATCH), ""});
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
    {
        std::filesystem::path model_dir = "./assets/models";
        int model_count = 0;

        if (std::filesystem::exists(model_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(model_dir)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".onnx" || ext == ".engine" || ext == ".trt") { model_count++; }
                }
            }
        }

        if (model_count > 0) {
            report.checks.push_back({"model_repository", CheckStatus::Ok,
                                     std::format("{} models found", model_count), ""});
        } else if (std::filesystem::exists(model_dir)) {
            report.checks.push_back({"model_repository", CheckStatus::Warn, "0 models found",
                                     "Run model download script"});
        } else {
            report.checks.push_back({"model_repository", CheckStatus::Fail, "Directory not found",
                                     "assets/models/ does not exist"});
        }
    }

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

        // Add provider field for onnxruntime
        if (c.name == "onnxruntime" && c.value.find("(") != std::string::npos) {
            auto pos = c.value.find("(");
            auto end = c.value.find(")");
            if (pos != std::string::npos && end != std::string::npos) {
                item["provider"] = c.value.substr(pos + 1, end - pos - 1);
            }
        }

        j["checks"].push_back(item);
    }
    j["summary"]["ok"] = report.ok_count;
    j["summary"]["warn"] = report.warn_count;
    j["summary"]["fail"] = report.fail_count;
    return j.dump(2);
}

} // namespace app::cli
