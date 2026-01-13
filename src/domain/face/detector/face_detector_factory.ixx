module;
#include <memory>

export module domain.face.detector:factory;

import :api;

export namespace domain::face::detector {

enum class DetectorType { Yolo, SCRFD, RetinaFace };

class FaceDetectorFactory {
public:
    static std::unique_ptr<IFaceDetector> create(DetectorType type);
};
} // namespace domain::face::detector
