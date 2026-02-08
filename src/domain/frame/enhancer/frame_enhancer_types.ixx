module;
#include <vector>
#include <opencv2/opencv.hpp>

export module domain.frame.enhancer:types;

export namespace domain::frame::enhancer {

enum class FrameEnhancerType : std::uint8_t { RealEsrGan, RealHatGan };

struct FrameEnhancerInput {
    cv::Mat target_frame;
    std::uint16_t blend{80};
};

} // namespace domain::frame::enhancer
