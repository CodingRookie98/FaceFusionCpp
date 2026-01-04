/**
 * @file           : core_options.ixx
 * @brief          : Defines configuration options for Core
 */

module;

export module core_options;
import logger;
import inference_session;

namespace ffc::core {
using namespace ffc::infra;
using namespace ai;

export struct CoreOptions {
    struct ModelOptions {
        bool force_download{true}; // 强制下载所有模型
        bool skip_download{false}; // 跳过下载模型

        bool operator==(const ModelOptions& other) const {
            return force_download == other.force_download && skip_download == other.skip_download;
        }
    };
    ModelOptions model_options{}; // 模型选项

    struct LoggerOptions {
        Logger::LogLevel log_level{Logger::LogLevel::Trace}; // 日志级别

        bool operator==(const LoggerOptions& other) const {
            return log_level == other.log_level;
        }
    };
    LoggerOptions logger_options{};

    struct TaskOptions {                         // 任务选项
        unsigned short per_task_thread_count{1}; // 指定进行任务的线程数量。减小此项可降低内存和显存占用。默认: 1

        bool operator==(const TaskOptions& other) const {
            return per_task_thread_count == other.per_task_thread_count;
        }
    };
    TaskOptions task_options{}; // 任务选项

    enum class MemoryStrategy {
        Strict,
        Tolerant,
    };
    struct MemoryOptions {                                                  // 内存选项
        MemoryStrategy processor_memory_strategy{MemoryStrategy::Tolerant}; // 处理器内存策略

        bool operator==(const MemoryOptions& other) const {
            return processor_memory_strategy == other.processor_memory_strategy;
        }
    };

    MemoryOptions memory_options{}; // 内存选项
    InferenceSession::Options inference_session_options{};

    bool operator==(const CoreOptions& other) const {
        return model_options == other.model_options && logger_options == other.logger_options && task_options == other.task_options && memory_options == other.memory_options && inference_session_options == other.inference_session_options;
    }
};

} // namespace ffc::core
