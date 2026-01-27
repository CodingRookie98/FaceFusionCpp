module;
#include <string>
#include <vector>
#include <unordered_set>

/**
 * @file concurrent_crypto.ixx
 * @brief Concurrent cryptographic operations
 * @author
 * CodingRookie
 * @date 2026-01-27
 */
export module foundation.infrastructure.concurrent_crypto;

export namespace foundation::infrastructure::concurrent_crypto {

/**
 * @brief Calculate SHA1 hash of multiple files combined concurrently
 * @param file_paths Set of file paths to hash
 * @return Combined SHA1 hash string
 */
std::string combined_sha1(const std::unordered_set<std::string>& file_paths);

/**
 * @brief Calculate SHA1 hash of multiple files concurrently
 * @param file_paths Set of file paths to hash
 * @return Vector of SHA1 hash strings
 */
std::vector<std::string> sha1_batch(const std::unordered_set<std::string>& file_paths);

} // namespace foundation::infrastructure::concurrent_crypto
