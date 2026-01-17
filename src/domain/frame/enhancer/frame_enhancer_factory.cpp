module;
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

module domain.frame.enhancer;

import :factory;
import :impl;
import :types;
import domain.ai.model_repository;

namespace domain::frame::enhancer {

std::shared_ptr<IFrameEnhancer> FrameEnhancerFactory::create(
    FrameEnhancerType type, const std::string& model_name,
    const foundation::ai::inference_session::Options& options) {
    auto repo = domain::ai::model_repository::ModelRepository::get_instance();
    std::string model_path = repo->ensure_model(model_name);

    int scale = 4;                             // default
    std::vector<int> tile_size = {256, 16, 8}; // default

    if (type == FrameEnhancerType::RealEsrGan) {
        if (model_name == "real_esrgan_x2" || model_name == "real_esrgan_x2_fp16") {
            scale = 2;
        } else if (model_name == "real_esrgan_x8" || model_name == "real_esrgan_x8_fp16") {
            scale = 8;
        } else if (model_name == "real_esrgan_x4" || model_name == "real_esrgan_x4_fp16") {
            scale = 4;
        } else {
            throw std::invalid_argument("model is not supported for RealEsrGan: " + model_name);
        }
    } else if (type == FrameEnhancerType::RealHatGan) {
        if (model_name != "real_hatgan_x4") {
            throw std::invalid_argument("model is not supported for RealHatGan: " + model_name);
        }
        // uses default scale 4, tile size default
    } else {
        return nullptr;
    }

    return std::make_shared<FrameEnhancerImpl>(model_path, options, tile_size, scale);
}

} // namespace domain::frame::enhancer
