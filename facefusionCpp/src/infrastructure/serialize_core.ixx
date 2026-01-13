module;
#include <unordered_set>
#include <nlohmann/json.hpp>

export module serialize:core;

import core_options;
import logger;
import inference_session;

export namespace ffc::infra {

using namespace core;
using json = nlohmann::json;
using namespace ai;

void to_json(json& j, CoreOptions::ModelOptions& model_options) {
    j = json{
        {"force_download", model_options.force_download},
        {"skip_download", model_options.skip_download},
    };
}

void from_json(json& j, CoreOptions::ModelOptions& model_options) {
    j.at("force_download").get_to(model_options.force_download);
    j.at("skip_download").get_to(model_options.skip_download);
}

NLOHMANN_JSON_SERIALIZE_ENUM(Logger::LogLevel, {
                                                   {Logger::LogLevel::Trace, "trace"},
                                                   {Logger::LogLevel::Debug, "debug"},
                                                   {Logger::LogLevel::Info, "info"},
                                                   {Logger::LogLevel::Warn, "warn"},
                                                   {Logger::LogLevel::Error, "error"},
                                                   {Logger::LogLevel::Critical, "critical"},
                                               })

void to_json(json& j, CoreOptions::LoggerOptions& logger_options) {
    json log_level_json;
    to_json(log_level_json, logger_options.log_level);
    j = json{
        {"log_level", log_level_json},
    };
}

void from_json(json& j, CoreOptions::LoggerOptions& logger_options) {
    from_json(j.at("log_level"), logger_options.log_level);
}

void to_json(json& j, CoreOptions::TaskOptions& task_options) {
    j = json{
        {"per_task_thread_count", task_options.per_task_thread_count},
    };
}

void from_json(json& j, CoreOptions::TaskOptions& task_options) {
    j.at("per_task_thread_count").get_to(task_options.per_task_thread_count);
}

NLOHMANN_JSON_SERIALIZE_ENUM(InferenceSession::ExecutionProvider,
                             {
                                 {InferenceSession::ExecutionProvider::CPU, "cpu"},
                                 {InferenceSession::ExecutionProvider::CUDA, "cuda"},
                                 {InferenceSession::ExecutionProvider::TensorRT, "tensor_rt"},
                             });

void to_json(json& j,
             std::unordered_set<InferenceSession::ExecutionProvider>& execution_providers) {
    for (auto provider : execution_providers) {
        json provider_json;
        to_json(provider_json, provider);
        j.push_back(provider_json);
    }
}

void from_json(json& j,
               std::unordered_set<InferenceSession::ExecutionProvider>& execution_providers) {
    for (auto& provider_json : j) {
        InferenceSession::ExecutionProvider provider;
        from_json(provider_json, provider);
        execution_providers.insert(provider);
    }
}

NLOHMANN_JSON_SERIALIZE_ENUM(CoreOptions::MemoryStrategy,
                             {
                                 {CoreOptions::MemoryStrategy::Strict, "strict"},
                                 {CoreOptions::MemoryStrategy::Tolerant, "tolerant"},
                             });

void to_json(json& j, CoreOptions::MemoryOptions& memory_options) {
    j = json{
        {"processor_memory_strategy", memory_options.processor_memory_strategy},
    };
}

void from_json(json& j, CoreOptions::MemoryOptions& memory_options) {
    memory_options.processor_memory_strategy =
        j.value("processor_memory_strategy", memory_options.processor_memory_strategy);
}

void to_json(json& j, InferenceSession::Options& inference_session_options) {
    j = json{
        {"device_id", inference_session_options.execution_device_id},
        {"providers", inference_session_options.execution_providers},
        {"enable_engine_cache", inference_session_options.enable_tensorrt_cache},
        {"enable_embed_engine", inference_session_options.enable_tensorrt_embed_engine},
        {"trt_max_workspace_size", inference_session_options.trt_max_workspace_size / (2 << 30)},
    };
}

void from_json(json& j, InferenceSession::Options& inference_session_options) {
    j.at("device_id").get_to(inference_session_options.execution_device_id);
    from_json(j.at("providers"), inference_session_options.execution_providers);
    j.at("enable_engine_cache").get_to(inference_session_options.enable_tensorrt_cache);
    j.at("enable_embed_engine").get_to(inference_session_options.enable_tensorrt_embed_engine);
    j.at("trt_max_workspace_size").get_to(inference_session_options.trt_max_workspace_size);
    inference_session_options.trt_max_workspace_size *= 2 << 30;
}

void to_json(json& j, CoreOptions& core_options) {
    json model_json, logger_json, task_json, memory_json, inference_session_json;
    to_json(model_json, core_options.model_options);
    to_json(logger_json, core_options.logger_options);
    to_json(task_json, core_options.task_options);
    to_json(memory_json, core_options.memory_options);
    to_json(inference_session_json, core_options.inference_session_options);
    j = json{
        {"model", model_json},
        {"log", logger_json},
        {"task", task_json},
        {"memory", memory_json},
        {"inference_session", inference_session_json},
    };
}

void from_json(json& j, CoreOptions& core_options) {
    from_json(j.at("model"), core_options.model_options);
    from_json(j.at("log"), core_options.logger_options);
    from_json(j.at("task"), core_options.task_options);
    from_json(j.at("memory"), core_options.memory_options);
    from_json(j.at("inference_session"), core_options.inference_session_options);
}

} // namespace ffc::infra
