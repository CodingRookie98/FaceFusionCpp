module;
#include <string>
#include <opencv2/core.hpp>

export module domain.face.enhancer:api;
import :types;
import foundation.ai.inference_session;

export namespace domain::face::enhancer {

class IFaceEnhancer {
public:
    virtual ~IFaceEnhancer() = default;

    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    virtual cv::Mat enhance_face(const EnhanceInput& input) = 0;
};

} // namespace domain::face::enhancer
