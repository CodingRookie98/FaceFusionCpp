/**
 * @file face_swapper_factory.ixx
 * @brief Factory for creating Face Swapper instances
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <memory>

export module domain.face.swapper:factory;

import :api;
import :inswapper;

export namespace domain::face::swapper {

/**
 * @brief Factory class for creating Face Swapper instances
 */
class FaceSwapperFactory {
public:
    /**
     * @brief Create an InSwapper instance
     * @return std::shared_ptr<IFaceSwapper> Pointer to the created swapper
     */
    static std::shared_ptr<IFaceSwapper> create_inswapper() {
        return std::make_shared<InSwapper>();
    }
};

} // namespace domain::face::swapper
