/**
 ******************************************************************************
 * @file           : frame_enhancer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-30
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

export module frame_enhancer:frame_enhancer_base;
import processor_base;

export namespace ffc::frame_enhancer {
class FrameEnhancerBase : public ProcessorBase {
public:
    FrameEnhancerBase() = default;
    ~FrameEnhancerBase() override = default;

    [[nodiscard]] std::string get_processor_name() const override = 0;

    void setTileSize(const std::vector<int>& size) {
        m_tileSize = size;
    }

    void setModelScale(const int& scale) {
        m_modelScale = scale;
    }

    [[nodiscard]] int getModelScale() const {
        return m_modelScale;
    }

protected:
    std::vector<int> m_tileSize;
    unsigned short m_modelScale{0};

    [[nodiscard]] static cv::Mat blend_frame(const cv::Mat& tempFrame, const cv::Mat& mergedFrame, const int& blend);
    static std::vector<float> get_input_data(const cv::Mat& frame);
    static cv::Mat get_output_data(const float* outputData, const cv::Size& size);
};

} // namespace ffc::frame_enhancer
