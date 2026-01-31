
module;
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>

module foundation.infrastructure.crypto;

namespace foundation::infrastructure::crypto {

namespace {
std::string bytes_to_hex(const unsigned char* bytes, size_t length) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) { ss << std::setw(2) << static_cast<int>(bytes[i]); }
    return ss.str();
}
} // namespace

std::string sha1(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) { throw std::runtime_error("Failed to open file for SHA1: " + file_path); }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestInit_ex(mdctx, md, NULL);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) { EVP_DigestUpdate(mdctx, buffer, file.gcount()); }
    EVP_DigestUpdate(mdctx, buffer, file.gcount());

    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    return bytes_to_hex(md_value, md_len);
}

std::string sha1_string(const std::string& input) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input.c_str(), input.size());
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    return bytes_to_hex(md_value, md_len);
}

std::string combined_sha1(const std::unordered_set<std::string>& file_paths) {
    if (file_paths.empty()) { return ""; }

    std::vector<std::string> sorted_paths(file_paths.begin(), file_paths.end());
    std::sort(sorted_paths.begin(), sorted_paths.end());

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestInit_ex(mdctx, md, NULL);

    for (const auto& path : sorted_paths) {
        std::string file_hash = sha1(path);
        EVP_DigestUpdate(mdctx, file_hash.c_str(), file_hash.size());
    }

    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    return bytes_to_hex(md_value, md_len);
}

} // namespace foundation::infrastructure::crypto
