module;
#include <string>
#include <unordered_set>

/**
 * @file crypto.ixx
 * @brief Cryptographic utilities
 * @author CodingRookie
 * @date 2026-01-18
 */
export module foundation.infrastructure.crypto;

export namespace foundation::infrastructure::crypto {

/**
 * @brief Calculate SHA1 hash of a single file
 * @param file_path Path to the file
 * @return SHA1 hash string
 */
std::string sha1(const std::string& file_path);

/**
 * @brief Calculate combined SHA1 hash of multiple files
 * @param file_paths Set of file paths to hash
 * @return Combined SHA1 hash string
 */
std::string combined_sha1(const std::unordered_set<std::string>& file_paths);

} // namespace foundation::infrastructure::crypto
