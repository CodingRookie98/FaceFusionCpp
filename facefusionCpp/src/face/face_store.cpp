/**
 ******************************************************************************
 * @file           : face_store.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-20
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <opencv2/opencv.hpp>
#include <openssl/sha.h>

module face_store;

namespace ffc {
FaceStore::FaceStore() = default;

void FaceStore::InsertFaces(const cv::Mat& frame, const std::vector<Face>& faces) {
    if (faces.empty()) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_facesMap[CreateFrameHash(frame)] = faces;
}

std::vector<Face> FaceStore::GetFaces(const cv::Mat& frame) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    const auto it = m_facesMap.find(CreateFrameHash(frame));
    if (it != m_facesMap.end()) {
        return it->second;
    }
    return {};
}

void FaceStore::ClearFaces() {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_facesMap.clear();
}

std::string FaceStore::CreateFrameHash(const cv::Mat& frame) {
    // 获取 Mat 数据的指针和大小
    const uchar* data = frame.data;
    const size_t dataSize = frame.total() * frame.elemSize();

    // 最终计算并获取结果
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data), dataSize, hash);
    // 将哈希结果转换为十六进制字符串
    std::ostringstream oss;
    for (const unsigned char& i : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i);
    }

    return oss.str();
}

void FaceStore::RemoveFaces(const std::string& facesName) {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_facesMap.erase(facesName);
}

void FaceStore::RemoveFaces(const cv::Mat& frame) {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_facesMap.erase(CreateFrameHash(frame));
}

void FaceStore::InsertFaces(const std::string& facesName, const std::vector<Face>& faces) {
    if (faces.empty()) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_facesMap[facesName] = faces;
}

std::vector<Face> FaceStore::GetFaces(const std::string& facesName) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    if (m_facesMap.contains(facesName)) {
        return m_facesMap[facesName];
    }
    return {};
}

FaceStore::~FaceStore() {
    ClearFaces();
}

bool FaceStore::IsContains(const cv::Mat& frame) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    return m_facesMap.contains(CreateFrameHash(frame));
}

bool FaceStore::IsContains(const std::string& facesName) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    return m_facesMap.contains(facesName);
}
} // namespace ffc