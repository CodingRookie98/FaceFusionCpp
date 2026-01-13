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
#include <opencv2/opencv.hpp>

export module domain.face.store;

import domain.face;

export namespace domain::face::store {

class FaceStore {
public:
    FaceStore();
    ~FaceStore();

    void remove_faces(const std::string& faces_name);
    void remove_faces(const cv::Mat& frame);
    void insert_faces(const cv::Mat& frame, const std::vector<Face>& faces);
    void insert_faces(const std::string& faces_name, const std::vector<Face>& faces);
    void clear_faces();
    [[nodiscard]] std::vector<Face> get_faces(const cv::Mat& frame);
    [[nodiscard]] std::vector<Face> get_faces(const std::string& faces_name);
    [[nodiscard]] static std::string create_frame_hash(const cv::Mat& frame);
    [[nodiscard]] bool is_contains(const cv::Mat& frame);
    [[nodiscard]] bool is_contains(const std::string& faces_name);

private:
    std::unordered_map<std::string, std::vector<Face>> m_faces_map;
    mutable std::shared_mutex m_rw_mutex;
};

} // namespace domain::face::store
