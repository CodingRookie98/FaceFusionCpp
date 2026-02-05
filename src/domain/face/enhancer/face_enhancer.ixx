module;
#include <memory>

export module domain.face.enhancer;

export import :api;
export import :types;
export import :factory;

export namespace domain::face::enhancer {
    using EnhancerType = FaceEnhancerFactory::Type;
    inline std::shared_ptr<IFaceEnhancer> create_enhancer(EnhancerType type) {
        return FaceEnhancerFactory::create(type);
    }
}
