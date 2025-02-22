/**
 ******************************************************************************
 * @file           : face_store.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-20
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <opencv2/opencv.hpp>

export module face_store;
export import face;

export class FaceStore {
public:
    FaceStore();
    ~FaceStore();

    void RemoveFaces(const std::string& facesName);
    void RemoveFaces(const cv::Mat& frame);
    void InsertFaces(const cv::Mat& frame, const std::vector<Face>& faces);
    void InsertFaces(const std::string& facesName, const std::vector<Face>& faces);
    void ClearFaces();
    std::vector<Face> GetFaces(const cv::Mat& frame);
    std::vector<Face> GetFaces(const std::string& facesName);
    static std::string CreateFrameHash(const cv::Mat& frame);
    bool IsContains(const cv::Mat& frame);
    bool IsContains(const std::string& facesName);

private:
    std::unordered_map<std::string, std::vector<Face>> m_facesMap;
    std::shared_mutex m_rwMutex;
};
