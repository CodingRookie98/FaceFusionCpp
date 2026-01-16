module;
#include <memory>

export module domain.face.expression:factory;

import :api;
import :live_portrait;
import foundation.ai.inference_session;

export namespace domain::face::expression {

/**
 * @brief Factory to create a LivePortrait expression restorer
 */
[[nodiscard]] std::unique_ptr<IFaceExpressionRestorer> create_live_portrait_restorer() {
    return std::make_unique<LivePortrait>();
}

} // namespace domain::face::expression
