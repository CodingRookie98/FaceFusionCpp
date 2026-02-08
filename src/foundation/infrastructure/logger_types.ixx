module;
#include <string>
#include <cstdint>
#include <regex>
#include <stdexcept>
#include <algorithm>

export module foundation.infrastructure.logger:types;

export namespace foundation::infrastructure::logger {

/**
 * @brief Log levels supported by the logger
 */
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warn, Error, Critical, Off };

/**
 * @brief Log rotation policy
 */
enum class RotationPolicy : std::uint8_t {
    Daily,  // Rotate daily at 00:00
    Hourly, // Rotate every hour
    Size    // Rotate by file size
};

/**
 * @brief Logger configuration structure
 * @details Corresponds to the logging node in app_config.yaml
 */
struct LoggingConfig {
    LogLevel level{LogLevel::Info};                 // Log level
    std::string directory{"./logs"};                // Log directory
    RotationPolicy rotation{RotationPolicy::Daily}; // Rotation policy
    uint32_t max_files{7};                          // Max retained files
    uint64_t max_total_size_bytes{1ULL << 30};      // Max total size (default 1GB)

    // Size mode specific parameters
    uint64_t max_file_size_bytes{10ULL << 20}; // Max single file size (default 10MB)
};

/**
 * @brief Parse size string (e.g., "1GB", "512MB", "100KB")
 * @param size_str Size string
 * @return Bytes count
 * @throws std::invalid_argument if format is invalid
 */
uint64_t parse_size_string(const std::string& size_str);

} // namespace foundation::infrastructure::logger
