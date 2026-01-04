/**
 ******************************************************************************
 * @file           : processor_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-20
 ******************************************************************************
 */
module;
#include <string>

export module processor_base;

namespace ffc {
export class ProcessorBase {
public:
    ProcessorBase() = default;
    virtual ~ProcessorBase() = default;

    [[nodiscard]] virtual std::string get_processor_name() const = 0;
};
} // namespace ffc