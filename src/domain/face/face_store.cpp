/**
 ******************************************************************************
 * @file           : face_store.cpp
 * @brief          : Face store module implementation
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <opencv2/opencv.hpp>
#include <openssl/sha.h> // Keep for SHA1 compatibility
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <sstream>
#include <list>
#include <utility>

module domain.face.store;

import domain.face;

namespace domain::face::store {

// FNV-1a 64-bit Hash Implementation
static uint64_t fnv1a_hash(const uchar* data, size_t len) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

FaceStore::FaceStore(FaceStoreOptions options) : m_options(options) {}

FaceStore::~FaceStore() {
    clear_faces();
}

void FaceStore::update_access(const std::string& key) {
    if (!m_options.enable_lru) return;

    // Assumes mutex is locked by caller
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Move to front of list (MRU)
        m_lru_list.splice(m_lru_list.begin(), m_lru_list, it->second.lru_iterator);
    }
}

void FaceStore::evict_if_needed() {
    if (!m_options.enable_lru || m_options.max_capacity == 0) return;

    while (m_cache.size() >= m_options.max_capacity) {
        // Evict LRU (back of list)
        if (m_lru_list.empty()) break;

        std::string lru_key = m_lru_list.back();
        m_lru_list.pop_back();
        m_cache.erase(lru_key);
    }
}

void FaceStore::insert_faces(const cv::Mat& frame, const std::vector<Face>& faces) {
    if (faces.empty()) { return; }
    std::string key = get_key(frame);

    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);

    if (m_cache.contains(key)) {
        m_cache[key].faces = faces;
        update_access(key);
    } else {
        evict_if_needed();
        m_lru_list.push_front(key);
        m_cache[key] = {faces, m_lru_list.begin()};
    }
}

void FaceStore::insert_faces(const std::string& faces_name, const std::vector<Face>& faces) {
    if (faces.empty()) { return; }
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);

    if (m_cache.contains(faces_name)) {
        m_cache[faces_name].faces = faces;
        update_access(faces_name);
    } else {
        evict_if_needed();
        m_lru_list.push_front(faces_name);
        m_cache[faces_name] = {faces, m_lru_list.begin()};
    }
}

std::vector<Face> FaceStore::get_faces(const cv::Mat& frame) {
    std::string key = get_key(frame);

    if (m_options.enable_lru) {
        std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
        if (auto it = m_cache.find(key); it != m_cache.end()) {
            update_access(key);
            return it->second.faces;
        }
    } else {
        std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
        if (auto it = m_cache.find(key); it != m_cache.end()) { return it->second.faces; }
    }
    return {};
}

std::vector<Face> FaceStore::get_faces(const std::string& faces_name) {
    if (m_options.enable_lru) {
        std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
        if (auto it = m_cache.find(faces_name); it != m_cache.end()) {
            update_access(faces_name);
            return it->second.faces;
        }
    } else {
        std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
        if (auto it = m_cache.find(faces_name); it != m_cache.end()) { return it->second.faces; }
    }
    return {};
}

void FaceStore::clear_faces() {
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    m_cache.clear();
    m_lru_list.clear();
}

void FaceStore::remove_faces(const std::string& faces_name) {
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    if (auto it = m_cache.find(faces_name); it != m_cache.end()) {
        m_lru_list.erase(it->second.lru_iterator);
        m_cache.erase(it);
    }
}

void FaceStore::remove_faces(const cv::Mat& frame) {
    std::string key = get_key(frame);
    remove_faces(key);
}

bool FaceStore::is_contains(const cv::Mat& frame) const {
    std::string key = get_key(frame);
    std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
    return m_cache.contains(key);
}

bool FaceStore::is_contains(const std::string& faces_name) const {
    std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
    return m_cache.contains(faces_name);
}

std::string FaceStore::calculate_hash(const cv::Mat& frame, HashStrategy strategy) {
    const uchar* data = frame.data;
    const size_t data_size = frame.total() * frame.elemSize();

    if (strategy == HashStrategy::FNV1a) {
        uint64_t hash = fnv1a_hash(data, data_size);
        std::ostringstream oss;
        oss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    } else {
        // SHA1
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(data), data_size, hash);
        std::ostringstream oss;
        for (const unsigned char& i : hash) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i);
        }
        return oss.str();
    }
}

std::string FaceStore::get_key(const cv::Mat& frame) const {
    return calculate_hash(frame, m_options.hash_strategy);
}

} // namespace domain::face::store
