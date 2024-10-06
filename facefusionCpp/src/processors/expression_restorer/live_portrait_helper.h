/**
 ******************************************************************************
 * @file           : live_portrai_helper.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_LIVE_PORTRAIT_HELPER_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_LIVE_PORTRAIT_HELPER_H_

#include <opencv2/opencv.hpp>

class LivePortraitHelper {
public:
    static cv::Mat createRotationMat(const float pitch, const float yaw, const float roll);
    static cv::Mat limitExpression(const cv::Mat &expression);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_PROCESSORS_EXPRESSION_RESTORER_LIVE_PORTRAIT_HELPER_H_
