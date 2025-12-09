/**
 * @file           : core_options.ixx
 * @brief          : Defines configuration options for Core
 */

module;
#include <vector>

export module core_options;
import logger;
import inference_session;

namespace ffc {

export struct CoreOptions {
    // misc
    bool force_download{true};
    bool skip_download{false};
    Logger::LogLevel log_level{Logger::LogLevel::Trace};
    // execution
    unsigned short execution_thread_count{1};
    // memory
    enum class MemoryStrategy {
        Strict,
        Tolerant,
    };
    MemoryStrategy processor_memory_strategy{MemoryStrategy::Tolerant};
    InferenceSession::Options inference_session_options{};
};

} // namespace ffc
