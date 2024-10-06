/**
 ******************************************************************************
 * @file           : face_store.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-20
 ******************************************************************************
 */

#include "face_store.h"
#include <openssl/sha.h>

std::shared_ptr<FaceStore> FaceStore::getInstance() {
    static std::shared_ptr<FaceStore> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<FaceStore>(); });
    return instance;
}

FaceStore::FaceStore() {
    m_faces = std::make_unique<std::unordered_map<std::string, std::vector<Face>>>();
}

void FaceStore::appendFaces(const cv::Mat &frame, const std::vector<Face> &faces) {
    if (faces.empty()) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    (*m_faces)[createFrameHash(frame)] = faces;
}

std::vector<Face> FaceStore::getFaces(const cv::Mat &visionFrame) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    auto it = m_faces->find(createFrameHash(visionFrame));
    if (it != m_faces->end()) {
        return it->second;
    }
    return {};
}

void FaceStore::clearFaces() {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_faces->clear();
}

std::string FaceStore::createFrameHash(const cv::Mat &visionFrame) {
    // 获取 Mat 数据的指针和大小
    const uchar *data = visionFrame.data;
    size_t dataSize = visionFrame.total() * visionFrame.elemSize();

    // 最终计算并获取结果
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(data), dataSize, hash);
    // 将哈希结果转换为十六进制字符串
    std::ostringstream oss;
    for (unsigned char i : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)i;
    }

    return oss.str();
}

void FaceStore::removeFaces(const std::string &facesName) {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_faces->erase(facesName);
}

void FaceStore::removeFaces(const cv::Mat &frame) {
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    m_faces->erase(createFrameHash(frame));
}

void FaceStore::appendFaces(const std::string &facesName, const std::vector<Face> &faces) {
    if (faces.empty()) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(m_rwMutex);
    (*m_faces)[facesName] = faces;
}

std::vector<Face> FaceStore::getFaces(const std::string &facesName) {
    std::shared_lock<std::shared_mutex> lock(m_rwMutex);
    auto it = m_faces->find(facesName);
    if (it != m_faces->end()) {
        return it->second;
    }
    return {};
}

FaceStore::~FaceStore() {
    clearFaces();
}