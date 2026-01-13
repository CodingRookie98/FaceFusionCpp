module;
#include <memory>

export module domain.face.detector:internal_creators;

import :api;

namespace domain::face::detector {
std::unique_ptr<IFaceDetector> create_yolo_detector();
std::unique_ptr<IFaceDetector> create_scrfd_detector();
std::unique_ptr<IFaceDetector> create_retina_detector();
} // namespace domain::face::detector
