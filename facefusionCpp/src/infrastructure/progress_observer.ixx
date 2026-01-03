/**
 * @file           : progress_observer.ixx
 * @brief          : Interface for progress observation
 */

module;
#include <string>

export module progress_observer;

namespace ffc::infra {

export struct IProgressObserver {
    virtual ~IProgressObserver() = default;
    virtual void on_start(int total_steps) = 0;
    virtual void on_progress(int step, const std::string& message) = 0;
    virtual void on_error(const std::string& error_msg) = 0;
    virtual void on_complete() = 0;
};

} // namespace ffc::infra
