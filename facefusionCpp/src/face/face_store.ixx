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

    void removeFaces(const std::string &facesName);
    void removeFaces(const cv::Mat &frame);
    void appendFaces(const cv::Mat &frame, const std::vector<Face> &faces);
    void appendFaces(const std::string &facesName, const std::vector<Face> &faces);
    void clearFaces();
    std::vector<Face> getFaces(const cv::Mat &frame);
    std::vector<Face> getFaces(const std::string &facesName);
    static std::string createFrameHash(const cv::Mat &frame);
    bool isContains(const cv::Mat &frame);
    bool isContains(const std::string &facesName);

private:
    std::unordered_map<std::string, std::vector<Face>> m_facesMap;
    std::shared_mutex m_rwMutex;
};
