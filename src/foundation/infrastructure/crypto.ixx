module;
#include <string>
#include <unordered_set>

export module foundation.infrastructure.crypto;

// import <string>;
// import <unordered_set>;

export namespace foundation::infrastructure::crypto {
std::string sha1(const std::string& file_path);
std::string combined_sha1(const std::unordered_set<std::string>& file_paths);
} // namespace foundation::infrastructure::crypto
