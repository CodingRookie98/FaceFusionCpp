module;
#include <memory>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

export module domain.frame.enhancer:impl;

import :api;
import :types;
import foundation.ai.inference_session;

export namespace domain::frame::enhancer {

class FrameEnhancerImpl : public IFrameEnhancer {
public:
    FrameEnhancerImpl(const std::string& model_path,
                      const foundation::ai::inference_session::Options& options,
                      const std::vector<int>& tile_size, int model_scale);

    [[nodiscard]] cv::Mat enhance_frame(const FrameEnhancerInput& input) const override;

private:
    std::vector<int> m_tile_size;
    int m_model_scale;
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;

    [[nodiscard]] static cv::Mat blend_frame(const cv::Mat& temp_frame, const cv::Mat& merged_frame,
                                             int blend);
    [[nodiscard]] static std::vector<float> get_input_data(const cv::Mat& frame);
    [[nodiscard]] static cv::Mat get_output_data(const float* output_data, const cv::Size& size);
};

} // namespace domain::frame::enhancer
