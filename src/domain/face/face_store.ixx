/**
 * @file face_store.ixx
 * @brief Thread-safe LRU cache for detected faces in frames
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <cstdint>
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

/**
 * @brief Hashing strategy for image frames
 */
enum class HashStrategy : std::uint8_t {
    SHA1, ///< cryptographic SHA1 hash (slow, accurate)
    FNV1a ///< Fast Non-Volatile hash (fast, good for small-medium sets)
};

/**
 * @brief Configuration for FaceStore cache
 */
struct FaceStoreOptions {
    HashStrategy hash_strategy = HashStrategy::FNV1a; ///< Strategy for key generation
    size_t max_capacity = 1000;                       ///< Max frames to store (0 = unlimited)
    bool enable_lru = true;                           ///< Whether to use LRU eviction policy
};

/**
 * @brief Thread-safe cache for detected face objects
 * @details Uses a hash of the frame content as key to avoid redundant face detection.
 *          Supports singleton access and LRU eviction.
 */
class FaceStore {
public:
    /**
     * @brief Get the singleton instance of FaceStore
     */
    static std::shared_ptr<FaceStore> get_instance();

    /**
     * @brief Construct a FaceStore with specific options
     */
    explicit FaceStore(FaceStoreOptions options = {});
    ~FaceStore();

    /**
     * @brief Remove faces associated with a key name
     */
    void remove_faces(const std::string& faces_name);

    /**
     * @brief Remove faces associated with an image frame
     */
    void remove_faces(const cv::Mat& frame);

    /**
     * @brief Insert detected faces for a given frame
     */
    void insert_faces(const cv::Mat& frame, const std::vector<Face>& faces);

    /**
     * @brief Insert faces associated with a specific key name
     */
    void insert_faces(const std::string& faces_name, const std::vector<Face>& faces);

    /**
     * @brief Clear all cached faces
     */
    void clear_faces();

    /**
     * @brief Retrieve faces for a frame (if cached)
     * @return Vector of Face objects, empty if not found
     */
    [[nodiscard]] std::vector<Face> get_faces(const cv::Mat& frame);

    /**
     * @brief Retrieve faces for a key name
     */
    [[nodiscard]] std::vector<Face> get_faces(const std::string& faces_name);

    /**
     * @brief Check if faces for a frame are cached
     */
    [[nodiscard]] bool is_contains(const cv::Mat& frame) const;

    /**
     * @brief Check if faces for a key name are cached
     */
    [[nodiscard]] bool is_contains(const std::string& faces_name) const;

    /**
     * @brief Calculate a unique hash string for an image
     * @param frame Input image
     * @param strategy Hashing algorithm to use
     * @return Unique identifier string
     */
    [[nodiscard]] static std::string calculate_hash(const cv::Mat& frame, HashStrategy strategy);

private:
    FaceStoreOptions m_options;

    std::list<std::string> m_lru_list;

    using AccessIterator = std::list<std::string>::iterator;
    struct CacheEntry {
        std::vector<Face> faces;
        AccessIterator lru_iterator;
    };

    std::unordered_map<std::string, CacheEntry> m_cache;

    mutable std::shared_mutex m_rw_mutex;

    void update_access(const std::string& key);
    void evict_if_needed();
    std::string get_key(const cv::Mat& frame) const;
};

} // namespace domain::face::store
