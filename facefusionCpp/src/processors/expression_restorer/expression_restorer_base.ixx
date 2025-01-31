/**
 ******************************************************************************
 * @file           : exprssion_restorer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

module;
#include <memory>
#include <string>

export module expression_restorer:expression_restorer_basee;
export import face_masker_hub;
import processor_base;

namespace ffc::expressionRestore {

using namespace faceMasker;

export class ExpressionRestorerBase : public ProcessorBase {
public:
    explicit ExpressionRestorerBase() = default;
    ~ExpressionRestorerBase() override = default;

    [[nodiscard]] std::string getProcessorName() const override = 0;

    void setFaceMaskers(const std::shared_ptr<FaceMaskerHub> &faceMaskerHub) {
        m_faceMaskerHub = faceMaskerHub;
    };

    [[nodiscard]] bool hasFaceMaskers() const {
        if (m_faceMaskerHub == nullptr) {
            return false;
        }
        return true;
    }

protected:
    std::shared_ptr<FaceMaskerHub> m_faceMaskerHub;
};
} // namespace ffc::expressionRestore
