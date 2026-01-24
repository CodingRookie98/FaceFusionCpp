/**
 ******************************************************************************
 * @file           : face_store.ixx
 * @brief          : Face store module interface
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include <list>
#include <utility>
#include <opencv2/opencv.hpp>

export module domain.face.store;

import domain.face;

export namespace domain::face::store {

enum class HashStrategy {
    SHA1,
    FNV1a // Lightweight replacement for xxHash/Murmur if external lib is not available
};

struct FaceStoreOptions {
    HashStrategy hash_strategy = HashStrategy::FNV1a;
    size_t max_capacity = 1000; // 0 for unlimited
    bool enable_lru = true;
};

class FaceStore {
public:
    static std::shared_ptr<FaceStore> get_instance();

    explicit FaceStore(FaceStoreOptions options = {});
    ~FaceStore();

    void remove_faces(const std::string& faces_name);
    void remove_faces(const cv::Mat& frame);
    void insert_faces(const cv::Mat& frame, const std::vector<Face>& faces);
    void insert_faces(const std::string& faces_name, const std::vector<Face>& faces);
    void clear_faces();

    [[nodiscard]] std::vector<Face> get_faces(const cv::Mat& frame);
    [[nodiscard]] std::vector<Face> get_faces(const std::string& faces_name);

    [[nodiscard]] bool is_contains(const cv::Mat& frame) const;
    [[nodiscard]] bool is_contains(const std::string& faces_name) const;

    // Helper to generate hash based on strategy (can be used externally if needed)
    [[nodiscard]] static std::string calculate_hash(const cv::Mat& frame, HashStrategy strategy);

private:
    FaceStoreOptions m_options;

    // LRU Cache Structure:
    // List stores keys in order of access (MRU at front, LRU at back)
    std::list<std::string> m_lru_list;

    // Map stores Key -> {Faces, Iterator to list}
    using AccessIterator = std::list<std::string>::iterator;
    struct CacheEntry {
        std::vector<Face> faces;
        AccessIterator lru_iterator;
    };

    std::unordered_map<std::string, CacheEntry> m_cache;

    mutable std::shared_mutex m_rw_mutex;

    // Internal helpers
    void update_access(const std::string& key);
    void evict_if_needed();
    std::string get_key(const cv::Mat& frame) const;
};

} // namespace domain::face::store
