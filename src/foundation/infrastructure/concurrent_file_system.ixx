module;
#include <string>
#include <vector>

/**
 * @file concurrent_file_system.ixx
 * @brief Concurrent file system operations
 * @author CodingRookie
 * @date 2026-01-18
 */
export module foundation.infrastructure.concurrent_file_system;

export namespace foundation::infrastructure::concurrent_file_system {

/**
 * @brief Remove multiple files concurrently
 * @param paths A vector of paths to files to remove
 */
void remove_files(const std::vector<std::string>& paths);

/**
 * @brief Copy multiple files concurrently to destinations
 * @param sources A vector of source file paths
 * @param destinations A vector of destination file paths (must match sources size)
 */
void copy_files(const std::vector<std::string>& sources,
                const std::vector<std::string>& destinations);
} // namespace foundation::infrastructure::concurrent_file_system
