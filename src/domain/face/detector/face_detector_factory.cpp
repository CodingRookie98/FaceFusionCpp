module domain.face.detector;

import :factory;
import :internal_creators;

namespace domain::face::detector {

std::unique_ptr<IFaceDetector> FaceDetectorFactory::create(DetectorType type) {
    switch (type) {
    case DetectorType::Yolo: return create_yolo_detector();
    case DetectorType::SCRFD: return create_scrfd_detector();
    case DetectorType::RetinaFace: return create_retina_detector();
    }
    return nullptr;
}
} // namespace domain::face::detector
