module;
#include <memory>
#include <string>

export module domain.frame.enhancer:factory;

import :api;
import :types;
import foundation.ai.inference_session;

export namespace domain::frame::enhancer {

class FrameEnhancerFactory {
public:
    static std::shared_ptr<IFrameEnhancer> create(
        FrameEnhancerType type, const std::string& model_name,
        const foundation::ai::inference_session::Options& options);
};

} // namespace domain::frame::enhancer
