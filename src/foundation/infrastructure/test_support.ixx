module;
#include <filesystem>
#include <string>

/**
 * @file test_support.ixx
 * @brief Test support utilities for Foundation layer
 * @author CodingRookie
 * @date 2026-01-18
 */
export module foundation.infrastructure.test_support;

export import foundation.infrastructure.core_utils;
export import foundation.infrastructure.network;
export import foundation.infrastructure.file_system;
export import foundation.infrastructure.concurrent_file_system;
export import foundation.infrastructure.crypto;
export import foundation.infrastructure.concurrent_crypto;
export import foundation.infrastructure.logger;
export import foundation.infrastructure.progress;
export import foundation.infrastructure.thread_pool;

export namespace foundation::infrastructure::test {

/**
 * @brief Reset test environment (clean up temp files etc.)
 */
void reset_environment();

/**
 * @brief Get the absolute path of the assets directory
 * @details Search order: env FACEFUSION_ASSETS_PATH -> current dir -> parent dir recursive
 * @return Path to assets directory
 */
std::filesystem::path get_assets_path();

/**
 * @brief Get absolute path for specific test data
 * @param relative_path Path relative to assets directory
 * @return Absolute path to test data file
 */
std::filesystem::path get_test_data_path(const std::string& relative_path);

} // namespace foundation::infrastructure::test
