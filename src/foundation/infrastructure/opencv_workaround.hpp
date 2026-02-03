#pragma once

// Patch for OpenCV 4.x compatibility with modern Clang and C++20
// This file should be included BEFORE any other OpenCV headers

#ifndef CV_DISABLE_S_CAST_HFLOAT
#define CV_DISABLE_S_CAST_HFLOAT
#endif

#ifndef CV_DO_NOT_INCLUDE_X86_INTRIN
#define CV_DO_NOT_INCLUDE_X86_INTRIN
#endif

#ifndef OPENCV_CAN_USE_FP16
#define OPENCV_CAN_USE_FP16 0
#endif

#ifndef CV_FP16
#define CV_FP16 0
#endif

#include <cstdint>
#include <cmath>
#include <climits>
#include <algorithm>
#include <type_traits>

// Include core headers to get the base definitions
#include <opencv2/core.hpp>
#include <opencv2/core/saturate.hpp>

// Fix for LP64 systems where 'long' might be distinct from both 'int' and 'long long'
namespace cv {
#if defined(__cplusplus) && __cplusplus >= 202002L
// We use a helper to avoid redefinition if long is already covered by int64_t or int
template <typename _Tp>
    requires(!std::is_same_v<long, int64_t> && !std::is_same_v<long, int32_t>)
static inline _Tp saturate_cast(long v) {
    return saturate_cast<_Tp>((long long)v);
}

template <typename _Tp>
    requires(!std::is_same_v<unsigned long, uint64_t> && !std::is_same_v<unsigned long, uint32_t>)
static inline _Tp saturate_cast(unsigned long v) {
    return saturate_cast<_Tp>((unsigned long long)v);
}
#endif
} // namespace cv
