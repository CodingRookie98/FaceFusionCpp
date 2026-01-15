module;
#include <memory>

export module domain.face.swapper:factory;

import :api;
import :inswapper;

export namespace domain::face::swapper {

class FaceSwapperFactory {
public:
    static std::shared_ptr<IFaceSwapper> create_inswapper() {
        return std::make_shared<InSwapper>();
    }
};

} // namespace domain::face::swapper
