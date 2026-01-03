/**
 * @file crypto.ixx
 * @brief Cryptographic functions module
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides SHA1 hash functions using OpenSSL
 */

module;
#include <string>
#include <unordered_set>

export module crypto;

namespace ffc::infra {
export namespace crypto {

/**
 * @brief Calculate SHA1 hash of a file
 * @param file_path Path to the file
 * @return std::string SHA1 hash in hexadecimal format
 * @note Uses OpenSSL library for SHA1 calculation
 */
std::string sha1(const std::string& file_path);

/**
 * @brief Calculate combined SHA1 hash of multiple files
 * @param file_paths Set of file paths to hash
 * @param use_thread_pool Whether to use thread pool for parallel hashing (default: true)
 * @return std::string Combined SHA1 hash in hexadecimal format
 * @note Hashes all files and combines the results into a single hash
 */
std::string combined_sha1(std::unordered_set<std::string>& file_paths,
                          const bool& use_thread_pool = true);
}
} // namespace ffc::infra::crypto
