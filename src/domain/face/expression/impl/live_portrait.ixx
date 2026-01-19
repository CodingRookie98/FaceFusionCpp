#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.expression:live_portrait;

import :api;
import :types;
import domain.face.helper;
import domain.face.masker;
import foundation.ai.inference_session;

export namespace domain::face::expression {

class LivePortrait final : public IFaceExpressionRestorer {
public:
    LivePortrait() = default;
    ~LivePortrait() override = default;

    void load_model(const std::string& feature_extractor_path,
                    const std::string& motion_extractor_path, const std::string& generator_path,
                    const foundation::ai::inference_session::Options& options = {}) override;

    cv::Mat restore_expression(const RestoreExpressionInput& input) override;

private:
    class FeatureExtractor {
    public:
        void load_model(const std::string& path,
                        const foundation::ai::inference_session::Options& options);
        [[nodiscard]] bool is_model_loaded() const;
        [[nodiscard]] std::vector<float> extract_feature(const cv::Mat& frame) const;

        [[nodiscard]] std::tuple<std::vector<float>, std::vector<int64_t>> prepare_input(
            const cv::Mat& frame) const;
        [[nodiscard]] std::vector<float> process_output(
            const std::vector<Ort::Value>& output_tensors) const;

    private:
        mutable foundation::ai::inference_session::InferenceSession m_session;
    };

    class MotionExtractor {
    public:
        void load_model(const std::string& path,
                        const foundation::ai::inference_session::Options& options);
        [[nodiscard]] bool is_model_loaded() const;
        [[nodiscard]] std::vector<std::vector<float>> extract_motion(const cv::Mat& frame) const;

        [[nodiscard]] std::tuple<std::vector<float>, std::vector<int64_t>> prepare_input(
            const cv::Mat& frame) const;
        [[nodiscard]] std::vector<std::vector<float>> process_output(
            const std::vector<Ort::Value>& output_tensors) const;

    private:
        mutable foundation::ai::inference_session::InferenceSession m_session;
    };

    class Generator {
    public:
        void load_model(const std::string& path,
                        const foundation::ai::inference_session::Options& options);
        [[nodiscard]] bool is_model_loaded() const;
        [[nodiscard]] cv::Mat generate_frame(std::vector<float>& feature_volume,
                                             std::vector<float>& source_motion_points,
                                             std::vector<float>& target_motion_points) const;
        [[nodiscard]] cv::Size get_output_size() const;

        [[nodiscard]] std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<float>,
                                 std::vector<int64_t>, std::vector<float>, std::vector<int64_t>>
        prepare_input(std::vector<float>& feature_volume, std::vector<float>& source_motion_points,
                      std::vector<float>& target_motion_points) const;
        [[nodiscard]] cv::Mat process_output(const std::vector<Ort::Value>& output_tensors) const;

    private:
        mutable foundation::ai::inference_session::InferenceSession m_session;
    };

private:
    FeatureExtractor m_feature_extractor;
    MotionExtractor m_motion_extractor;
    Generator m_generator;

    cv::Size m_generator_output_size{512, 512};
    domain::face::helper::WarpTemplateType m_warp_template_type =
        domain::face::helper::WarpTemplateType::Arcface_128_v2;

    // Helper functions
    [[nodiscard]] static std::vector<float> get_input_image_data(const cv::Mat& image,
                                                                 const cv::Size& size);
    [[nodiscard]] cv::Mat apply_restore(const cv::Mat& cropped_source_frame,
                                        const cv::Mat& cropped_target_frame,
                                        float restore_factor) const;
    [[nodiscard]] static cv::Mat create_rotation_mat(float pitch, float yaw, float roll);
    [[nodiscard]] static cv::Mat limit_expression(const cv::Mat& expression);
};

} // namespace domain::face::expression
