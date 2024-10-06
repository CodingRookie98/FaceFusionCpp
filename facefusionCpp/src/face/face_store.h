/**
 ******************************************************************************
 * @file           : face_store.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-20
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_FACE_STORE_H_
#define FACEFUSIONCPP_SRC_FACE_STORE_H_

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include "face.h"

class FaceStore {
public:
    FaceStore();
    ~FaceStore();
    FaceStore(const FaceStore &) = delete;
    FaceStore &operator=(const FaceStore &) = delete;
    FaceStore(FaceStore &&) = delete;
    FaceStore &operator=(FaceStore &&) = delete;

    static std::shared_ptr<FaceStore> getInstance();
    void removeFaces(const std::string &facesName);
    void removeFaces(const cv::Mat &frame);
    void appendFaces(const cv::Mat &frame, const std::vector<Face> &faces);
    void appendFaces(const std::string &facesName, const std::vector<Face> &faces);
    void clearFaces();
    std::vector<Face> getFaces(const cv::Mat &frame);
    std::vector<Face> getFaces(const std::string &facesName);
    static std::string createFrameHash(const cv::Mat &frame);
private:
    std::unique_ptr<std::unordered_map<std::string, std::vector<Face>>> m_faces;
    std::shared_mutex m_rwMutex;
};

#endif // FACEFUSIONCPP_SRC_FACE_STORE_H_
