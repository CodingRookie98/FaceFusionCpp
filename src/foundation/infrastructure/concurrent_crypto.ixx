module;
#include <string>
#include <vector>
#include <unordered_set>

export module foundation.infrastructure.concurrent_crypto;

// import <string>;
// import <vector>;
// import <unordered_set>;

export namespace foundation::infrastructure::concurrent_crypto {
std::string combined_sha1(const std::unordered_set<std::string>& file_paths);
std::vector<std::string> sha1_batch(const std::unordered_set<std::string>& file_paths);
} // namespace foundation::infrastructure::concurrent_crypto
