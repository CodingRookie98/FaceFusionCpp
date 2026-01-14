/**
 ******************************************************************************
 * @file           : face_store.cpp
 * @brief          : Face store module implementation
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <opencv2/opencv.hpp>
#include <openssl/sha.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <sstream>

module domain.face.store;

import domain.face;

namespace domain::face::store {

FaceStore::FaceStore() = default;

FaceStore::~FaceStore() {
    clear_faces();
}

void FaceStore::insert_faces(const cv::Mat& frame, const std::vector<Face>& faces) {
    if (faces.empty()) { return; }
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    m_faces_map[create_frame_hash(frame)] = faces;
}

void FaceStore::insert_faces(const std::string& faces_name, const std::vector<Face>& faces) {
    if (faces.empty()) { return; }
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    m_faces_map[faces_name] = faces;
}

std::vector<Face> FaceStore::get_faces(const cv::Mat& frame) {
    std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
    const auto it = m_faces_map.find(create_frame_hash(frame));
    if (it != m_faces_map.end()) { return it->second; }
    return {};
}

std::vector<Face> FaceStore::get_faces(const std::string& faces_name) {
    std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
    if (const auto it = m_faces_map.find(faces_name); it != m_faces_map.end()) {
        return it->second;
    }
    return {};
}

void FaceStore::clear_faces() {
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    m_faces_map.clear();
}

void FaceStore::remove_faces(const std::string& faces_name) {
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    m_faces_map.erase(faces_name);
}

void FaceStore::remove_faces(const cv::Mat& frame) {
    std::unique_lock<std::shared_mutex> lock(m_rw_mutex);
    m_faces_map.erase(create_frame_hash(frame));
}

bool FaceStore::is_contains(const cv::Mat& frame) const {
    std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
    return m_faces_map.contains(create_frame_hash(frame));
}

bool FaceStore::is_contains(const std::string& faces_name) const {
    std::shared_lock<std::shared_mutex> lock(m_rw_mutex);
    return m_faces_map.contains(faces_name);
}

std::string FaceStore::create_frame_hash(const cv::Mat& frame) {
    // 获取 Mat 数据的指针和大小
    const uchar* data = frame.data;
    const size_t data_size = frame.total() * frame.elemSize();

    // 最终计算并获取结果
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data), data_size, hash);

    // 将哈希结果转换为十六进制字符串
    std::ostringstream oss;
    for (const unsigned char& i : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i);
    }

    return oss.str();
}

} // namespace domain::face::store
