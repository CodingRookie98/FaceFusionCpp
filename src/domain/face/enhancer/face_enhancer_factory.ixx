module;
#include <memory>

export module domain.face.enhancer:factory;
import :api;

export namespace domain::face::enhancer {

class FaceEnhancerFactory {
public:
    enum class Type : std::uint8_t { CodeFormer, GfpGan };
    static std::shared_ptr<IFaceEnhancer> create(Type type);
};

} // namespace domain::face::enhancer
