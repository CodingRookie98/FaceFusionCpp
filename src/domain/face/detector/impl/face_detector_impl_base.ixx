module;
#include <opencv2/core/mat.hpp>

export module domain.face.detector:impl_base;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::detector {

    class FaceDetectorImplBase : public IFaceDetector, public foundation::ai::inference_session::InferenceSession {
    public:
        using foundation::ai::inference_session::InferenceSession::InferenceSession;
        
        virtual ~FaceDetectorImplBase() = default;
    };
}
