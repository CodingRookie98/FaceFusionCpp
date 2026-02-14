
module;

#include <cstdint>
#include <indicators/progress_bar.hpp>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

module foundation.infrastructure.progress;
import foundation.infrastructure.console;

namespace foundation::infrastructure::progress {

struct ProgressBar::Impl {
    indicators::ProgressBar bar;

    Impl(const std::string& postfix_text) :
        bar{indicators::option::BarWidth{50},
            indicators::option::Start{"["},
            indicators::option::Fill{"="},
            indicators::option::Lead{">"},
            indicators::option::Remainder{" "},
            indicators::option::End{"]"},
            indicators::option::PostfixText{postfix_text},
            indicators::option::ForegroundColor{indicators::Color::green},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
            indicators::option::FontStyles{
                std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}} {}
};

ProgressBar::ProgressBar(const std::string& postfix_text) :
    m_impl(std::make_unique<Impl>(postfix_text)) {
    foundation::infrastructure::console::ConsoleManager::instance().register_progress_bar(this);
}

ProgressBar::~ProgressBar() {
    foundation::infrastructure::console::ConsoleManager::instance().unregister_progress_bar(this);
}

ProgressBar::ProgressBar(ProgressBar&& other) noexcept : m_impl(std::move(other.m_impl)) {
    foundation::infrastructure::console::ConsoleManager::instance().register_progress_bar(this);
}

ProgressBar& ProgressBar::operator=(ProgressBar&& other) noexcept {
    if (this != &other) {
        foundation::infrastructure::console::ConsoleManager::instance().unregister_progress_bar(
            this);
        m_impl = std::move(other.m_impl);
        foundation::infrastructure::console::ConsoleManager::instance().register_progress_bar(this);
    }
    return *this;
}

void ProgressBar::set_progress(float percent) {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) m_impl->bar.set_progress(percent);
}

void ProgressBar::set_postfix_text(const std::string& text) {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) m_impl->bar.set_option(indicators::option::PostfixText{text});
}

void ProgressBar::tick() {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) m_impl->bar.tick();
}

bool ProgressBar::is_completed() const {
    // Read-only, maybe no lock needed if is_completed is atomic?
    // But to be safe and consistent
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) return m_impl->bar.is_completed();
    return false;
}

void ProgressBar::mark_as_completed() {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) m_impl->bar.mark_as_completed();
}

void ProgressBar::suspend() {
    // Clear line using ANSI escape code: \033[2K (clear line), \r (move to start)
    std::cout << "\033[2K\r" << std::flush;
}

void ProgressBar::resume() {
    if (m_impl) {
        m_impl->bar.print_progress();
        std::cout << std::flush;
    }
}

} // namespace foundation::infrastructure::progress
