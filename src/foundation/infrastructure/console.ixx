module;
#include <mutex>
#include <memory>
#include <iostream>
#include <atomic>

export module foundation.infrastructure.console;

export namespace foundation::infrastructure::console {

struct IProgressController {
    virtual void suspend() = 0;
    virtual void resume() = 0;
    virtual ~IProgressController() = default;
};

class ConsoleManager {
public:
    static ConsoleManager& instance();

    void register_progress_bar(IProgressController* controller);
    void unregister_progress_bar(IProgressController* controller);

    // Call these only when you hold the mutex (e.g. inside ScopedSuspend)
    void suspend_active();
    void resume_active();

    std::recursive_mutex& mutex();

private:
    ConsoleManager() = default;
    std::recursive_mutex m_mutex;
    IProgressController* m_active_controller{nullptr};
};

class ScopedSuspend {
public:
    ScopedSuspend();
    ~ScopedSuspend();
    
    // Allow move but not copy
    ScopedSuspend(ScopedSuspend&&) = default;
    ScopedSuspend& operator=(ScopedSuspend&&) = default;
    ScopedSuspend(const ScopedSuspend&) = delete;
    ScopedSuspend& operator=(const ScopedSuspend&) = delete;

private:
    std::unique_lock<std::recursive_mutex> m_lock;
};

}
