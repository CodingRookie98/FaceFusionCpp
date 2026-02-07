#pragma once

#include <cstdint>

namespace test_constants {

// ============================================================================
// Face Verification Thresholds
// ============================================================================
constexpr float FACE_SIMILARITY_THRESHOLD = 0.65f;  // ArcFace distance threshold

// ============================================================================
// Performance Thresholds (RTX 4060 8GB Baseline)
// ============================================================================
constexpr double MIN_FPS_RTX4060 = 15.0;
constexpr double FRAME_PASS_RATE = 0.9;  // 90% frame pass rate

// ============================================================================
// Resource Thresholds
// ============================================================================
constexpr double VRAM_THRESHOLD_GB = 6.5;
constexpr double MEMORY_DELTA_THRESHOLD_MB = 50.0;

// ============================================================================
// Timeout Thresholds (Milliseconds)
// ============================================================================
#ifdef NDEBUG
constexpr int64_t TIMEOUT_IMAGE_512_MS = 3000;
constexpr int64_t TIMEOUT_IMAGE_720P_MS = 10000;
constexpr int64_t TIMEOUT_IMAGE_2K_MS = 10000;
constexpr int64_t TIMEOUT_VIDEO_40S_MS = 40000;
#else
constexpr int64_t TIMEOUT_IMAGE_512_MS = 20000;
constexpr int64_t TIMEOUT_IMAGE_720P_MS = 20000;
constexpr int64_t TIMEOUT_IMAGE_2K_MS = 20000;
constexpr int64_t TIMEOUT_VIDEO_40S_MS = 120000; // Increased for debug overhead
#endif

}  // namespace test_constants
