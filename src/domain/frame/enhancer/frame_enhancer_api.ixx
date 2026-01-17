module;
#include <opencv2/opencv.hpp>

export module domain.frame.enhancer:api;

import :types;

export namespace domain::frame::enhancer {

class IFrameEnhancer {
public:
    virtual ~IFrameEnhancer() = default;

    [[nodiscard]] virtual cv::Mat enhance_frame(const FrameEnhancerInput& input) const = 0;
};

} // namespace domain::frame::enhancer
