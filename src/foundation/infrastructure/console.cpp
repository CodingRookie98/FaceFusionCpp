module;
#include <mutex>

module foundation.infrastructure.console;

namespace foundation::infrastructure::console {

ConsoleManager& ConsoleManager::instance() {
    static ConsoleManager instance;
    return instance;
}

void ConsoleManager::register_progress_bar(IProgressController* controller) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_active_controller = controller;
}

void ConsoleManager::unregister_progress_bar(IProgressController* controller) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_active_controller == controller) { m_active_controller = nullptr; }
}

void ConsoleManager::suspend_active() {
    // Assumes mutex is locked by caller (ScopedSuspend)
    if (m_active_controller != nullptr) { m_active_controller->suspend(); }
}

void ConsoleManager::resume_active() {
    // Assumes mutex is locked by caller (ScopedSuspend)
    if (m_active_controller != nullptr) { m_active_controller->resume(); }
}

std::recursive_mutex& ConsoleManager::mutex() {
    return m_mutex;
}

ScopedSuspend::ScopedSuspend() : m_lock(ConsoleManager::instance().mutex()) {
    ConsoleManager::instance().suspend_active();
}

ScopedSuspend::~ScopedSuspend() {
    ConsoleManager::instance().resume_active();
    // m_lock releases automatically
}

} // namespace foundation::infrastructure::console
