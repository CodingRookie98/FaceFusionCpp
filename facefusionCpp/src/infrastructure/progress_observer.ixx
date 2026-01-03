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
    virtual void onStart(int total_steps) = 0;
    virtual void onProgress(int step, const std::string& message) = 0;
    virtual void onError(const std::string& error_msg) = 0;
    virtual void onComplete() = 0;
};

} // namespace ffc::infra
