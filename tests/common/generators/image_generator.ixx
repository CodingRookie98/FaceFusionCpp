module;
#include <opencv2/core.hpp>

export module tests.common.generators.image_generator;

export namespace tests::common::generators {

cv::Mat create_black_image(int width, int height) {
    return cv::Mat::zeros(height, width, CV_8UC3);
}

} // namespace tests::common::generators
