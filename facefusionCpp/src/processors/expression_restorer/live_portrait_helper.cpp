/**
 ******************************************************************************
 * @file           : live_portrai_helper.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

#include "live_portrait_helper.h"

namespace Expression {
std::vector<float> expressionMin{
    -2.88067125e-02f, -8.12731311e-02f, -1.70541159e-03f,
    -4.88598682e-02f, -3.32196616e-02f, -1.67431499e-04f,
    -6.75425082e-02f, -4.28681746e-02f, -1.98950816e-04f,
    -7.23103955e-02f, -3.28503326e-02f, -7.31324719e-04f,
    -3.87073644e-02f, -6.01546466e-02f, -5.50269964e-04f,
    -6.38048723e-02f, -2.23840728e-01f, -7.13261834e-04f,
    -3.02710701e-02f, -3.93195450e-02f, -8.24086510e-06f,
    -2.95799859e-02f, -5.39318882e-02f, -1.74219604e-04f,
    -2.92359516e-02f, -1.53050944e-02f, -6.30460854e-05f,
    -5.56493877e-03f, -2.34344602e-02f, -1.26858242e-04f,
    -4.37593013e-02f, -2.77768299e-02f, -2.70503685e-02f,
    -1.76926646e-02f, -1.91676542e-02f, -1.15090821e-04f,
    -8.34268332e-03f, -3.99775570e-03f, -3.27481248e-05f,
    -3.40162888e-02f, -2.81868968e-02f, -1.96679524e-04f,
    -2.91855410e-02f, -3.97511162e-02f, -2.81230678e-05f,
    -1.50395725e-02f, -2.49494594e-02f, -9.42573533e-05f,
    -1.67938769e-02f, -2.00953931e-02f, -4.00750607e-04f,
    -1.86435618e-02f, -2.48535164e-02f, -2.74416432e-02f,
    -4.61211195e-03f, -1.21660791e-02f, -2.93173041e-04f,
    -4.10017073e-02f, -7.43824020e-02f, -4.42762971e-02f,
    -1.90370996e-02f, -3.74363363e-02f, -1.34740388e-02f},
    expressionMax{
        4.46682945e-02f, 7.08772913e-02f, 4.08344204e-04f,
        2.14308221e-02f, 6.15894832e-02f, 4.85319615e-05f,
        3.02363783e-02f, 4.45043296e-02f, 1.28298725e-05f,
        3.05869691e-02f, 3.79812494e-02f, 6.57040102e-04f,
        4.45670523e-02f, 3.97259220e-02f, 7.10966764e-04f,
        9.43699256e-02f, 9.85926315e-02f, 2.02551950e-04f,
        1.61131397e-02f, 2.92906128e-02f, 3.44733417e-06f,
        5.23825921e-02f, 1.07065082e-01f, 6.61510974e-04f,
        2.85718683e-03f, 8.32320191e-03f, 2.39314613e-04f,
        2.57947259e-02f, 1.60935968e-02f, 2.41853559e-05f,
        4.90833223e-02f, 3.43903080e-02f, 3.22353356e-02f,
        1.44766076e-02f, 3.39248963e-02f, 1.42291479e-04f,
        8.75749043e-04f, 6.82212645e-03f, 2.76097053e-05f,
        1.86958015e-02f, 3.84016186e-02f, 7.33085908e-05f,
        2.01714113e-02f, 4.90544215e-02f, 2.34028921e-05f,
        2.46518422e-02f, 3.29151377e-02f, 3.48571630e-05f,
        2.22457591e-02f, 1.21796541e-02f, 1.56396593e-04f,
        1.72109623e-02f, 3.01626958e-02f, 1.36556877e-02f,
        1.83460284e-02f, 1.61141958e-02f, 2.87440169e-04f,
        3.57594155e-02f, 1.80554688e-01f, 2.75554154e-02f,
        2.17450950e-02f, 8.66811201e-02f, 3.34241726e-02f};
} // namespace Expression

cv::Mat LivePortraitHelper::createRotationMat(float pitch, float yaw, float roll) {
    // Convert degrees to radians
    pitch = pitch * CV_PI / 180.0;
    yaw = yaw * CV_PI / 180.0;
    roll = roll * CV_PI / 180.0;

    // Create rotation matrices for each axis
    cv::Mat Rx = (cv::Mat_<float>(3, 3) << 1, 0, 0,
                  0, cos(pitch), -sin(pitch),
                  0, sin(pitch), cos(pitch));

    cv::Mat Ry = (cv::Mat_<float>(3, 3) << cos(yaw), 0, sin(yaw),
                  0, 1, 0,
                  -sin(yaw), 0, cos(yaw));

    cv::Mat Rz = (cv::Mat_<float>(3, 3) << cos(roll), -sin(roll), 0,
                  sin(roll), cos(roll), 0,
                  0, 0, 1);

    // Combine the rotation matrices: R = Rz * Ry * Rx
    cv::Mat R = Rz * Ry * Rx;

    return R;
}

cv::Mat LivePortraitHelper::limitExpression(const cv::Mat &expression) {
    cv::Mat limitedExpression;
    static cv::Mat expressionMinMat(21, 3, CV_32FC1, Expression::expressionMin.data());
    static cv::Mat expressionMaxMat(21, 3, CV_32FC1, Expression::expressionMax.data());
    cv::max(expression, expressionMinMat, limitedExpression);
    cv::min(limitedExpression, expressionMaxMat, limitedExpression);
    return limitedExpression;
}
