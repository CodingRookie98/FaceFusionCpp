module;
#include <vector>
#include <opencv2/opencv.hpp>

export module domain.frame.enhancer:types;

export namespace domain::frame::enhancer {

enum class FrameEnhancerType { RealEsrGan, RealHatGan };

struct FrameEnhancerInput {
    cv::Mat target_frame;
    unsigned short blend{80};
};

} // namespace domain::frame::enhancer
