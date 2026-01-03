/**
 ******************************************************************************
 * @file           : crypto.ixx
 * @author         : CodingRookie
 * @brief          : Cryptographic functions module
 * @attention      : None
 * @date           : 2025-01-02
 ******************************************************************************
 */

module;
#include <string>
#include <unordered_set>

export module crypto;

namespace ffc::infra {
export namespace crypto {
std::string sha1(const std::string& file_path);
std::string combined_sha1(std::unordered_set<std::string>& file_paths,
                          const bool& use_thread_pool = true);
} // namespace crypto
} // namespace ffc::infra
