
module;
#include <string>
#include <vector>
#include <unordered_set>
#include <future>
#include <algorithm>
#include <iostream>
// Re-include OpenSSL only if we need it for combination, but we can reuse crypto module if we implemented a string hasher there.
// But wait, crypto module only exports sha1(file) and combined_sha1(files).
// If we want to hash the combined hashes, we need a string hasher.
// I'll re-include OpenSSL here just for the string hashing part for combination.
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

module foundation.infrastructure.concurrent_crypto;

import foundation.infrastructure.crypto;
import foundation.infrastructure.thread_pool;

namespace foundation::infrastructure::concurrent_crypto {

namespace {
std::string bytes_to_hex(const unsigned char* bytes, size_t length) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return ss.str();
}
} // namespace

std::vector<std::string> sha1_batch(const std::unordered_set<std::string>& file_paths) {
    if (file_paths.empty()) {
        return {};
    }

    std::vector<std::string> sorted_paths(file_paths.begin(), file_paths.end());
    std::sort(sorted_paths.begin(), sorted_paths.end()); // Deterministic order

    std::vector<std::future<std::string>> futures;
    auto& pool = foundation::infrastructure::thread_pool::ThreadPool::instance();

    for (const auto& path : sorted_paths) {
        auto promise = std::make_shared<std::promise<std::string>>();
        futures.push_back(promise->get_future());

        pool.enqueue([path, promise]() {
            try {
                std::string hash = foundation::infrastructure::crypto::sha1(path);
                promise->set_value(hash);
            } catch (...) {
                promise->set_value(""); // Handle error gracefully or propagate
            }
        });
    }

    std::vector<std::string> results;
    results.reserve(sorted_paths.size());
    for (auto& f : futures) {
        results.push_back(f.get());
    }
    return results;
}

std::string combined_sha1(const std::unordered_set<std::string>& file_paths) {
    if (file_paths.empty()) return "";

    std::vector<std::string> hashes = sha1_batch(file_paths); // Already sorted inside sha1_batch

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md  = EVP_sha1();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestInit_ex(mdctx, md, NULL);

    for (const auto& hash : hashes) {
        EVP_DigestUpdate(mdctx, hash.c_str(), hash.size());
    }

    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    return bytes_to_hex(md_value, md_len);
}

} // namespace foundation::infrastructure::concurrent_crypto
