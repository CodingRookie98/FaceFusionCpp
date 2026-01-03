/**
 * @file crypto.cpp
 * @brief Cryptographic functions module implementation
 * @author CodingRookie
 * @date 2026-01-04
 * @note This file contains the implementation of the cryptographic functions module
 */

module;
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ranges>
#include <future>
#include <openssl/sha.h>

module crypto;
import file_system;
import thread_pool;

namespace ffc::infra::crypto {

std::string sha1(const std::string& file_path) {
    if (file_path.empty()) {
        return {};
    }
    if (!file_system::file_exists(file_path)) {
        return {};
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return "";
    }

    SHA_CTX sha1_ctx;
    if (!SHA1_Init(&sha1_ctx)) {
        std::cerr << "SHA1_Init failed" << std::endl;
        return "";
    }

    constexpr size_t buffer_size = 8192;
    std::vector<char> buffer(buffer_size);

    while (file.read(buffer.data(), buffer_size)) {
        if (!SHA1_Update(&sha1_ctx, buffer.data(), file.gcount())) {
            std::cerr << "SHA1_Update failed" << std::endl;
            return "";
        }
    }
    if (file.gcount() > 0) {
        if (!SHA1_Update(&sha1_ctx, buffer.data(), file.gcount())) {
            std::cerr << "SHA1_Update failed" << std::endl;
            return "";
        }
    }

    unsigned char sha1[SHA_DIGEST_LENGTH];
    if (!SHA1_Final(sha1, &sha1_ctx)) {
        std::cerr << "SHA1_Final failed" << std::endl;
        return "";
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : sha1) {
        oss << std::setw(2) << static_cast<int>(byte);
    }

    return oss.str();
}

std::string combined_sha1(std::unordered_set<std::string>& file_paths,
                         const bool& use_thread_pool) {
    if (file_paths.empty()) {
        return {};
    }

    std::vector<std::string> sha1_vec;
    if (use_thread_pool) {
        std::vector<std::future<std::string>> futures;
        for (auto& file_path : file_paths) {
            futures.emplace_back(ThreadPool::instance()->enqueue([file_path]() {
                return sha1(file_path);
            }));
        }

        for (auto& future : futures) {
            sha1_vec.emplace_back(future.get());
        }
    } else {
        for (auto& file_path : file_paths) {
            sha1_vec.emplace_back(sha1(file_path));
        }
    }
    std::ranges::sort(sha1_vec);

    std::string combined_sha1;
    for (const auto& sha1 : sha1_vec) {
        combined_sha1 += sha1;
    }

    unsigned char sha1[SHA_DIGEST_LENGTH];
    ::SHA1(reinterpret_cast<const unsigned char*>(combined_sha1.c_str()), combined_sha1.length(), sha1);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : sha1) {
        oss << std::setw(2) << static_cast<int>(byte);
    }

    return oss.str();
}

} // namespace ffc::crypto
