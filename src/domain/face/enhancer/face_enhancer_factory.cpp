module;
#include <memory>
#include <stdexcept>

module domain.face.enhancer;
import :code_former;
import :gfp_gan;

namespace domain::face::enhancer {

std::shared_ptr<IFaceEnhancer> FaceEnhancerFactory::create(Type type) {
    switch (type) {
    case Type::CodeFormer: return std::make_shared<CodeFormer>();
    case Type::GfpGan: return std::make_shared<GfpGan>();
    default: throw std::invalid_argument("Unknown FaceEnhancer type");
    }
}

} // namespace domain::face::enhancer
