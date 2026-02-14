
module;

#include <cstdint>
#include <indicators/progress_bar.hpp>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <sstream>
#include <iostream>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

module foundation.infrastructure.progress;
import foundation.infrastructure.console;

namespace foundation::infrastructure::progress {

int get_terminal_width() {
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#endif
}

struct ProgressBar::Impl {
    indicators::ProgressBar bar;
    std::stringstream buffer;

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
                std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
            indicators::option::Stream{buffer}} {}

    void flush_buffer() {
        std::string content = buffer.str();
        buffer.str("");
        buffer.clear();

        if (content.empty()) return;

        while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
            content.pop_back();
        }

        int width = get_terminal_width();
        if (width <= 0) width = 80;

        std::cout << "\r" << content << "\033[K" << std::flush;
    }
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
    if (m_impl) {
        m_impl->bar.set_progress(percent);
        m_impl->flush_buffer();
    }
}

void ProgressBar::set_postfix_text(const std::string& text) {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) { m_impl->bar.set_option(indicators::option::PostfixText{text}); }
}

void ProgressBar::tick() {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) {
        m_impl->bar.tick();
        m_impl->flush_buffer();
    }
}

bool ProgressBar::is_completed() const {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) return m_impl->bar.is_completed();
    return false;
}

void ProgressBar::mark_as_completed() {
    std::lock_guard<std::recursive_mutex> lock(
        foundation::infrastructure::console::ConsoleManager::instance().mutex());
    if (m_impl) {
        m_impl->bar.mark_as_completed();
        m_impl->flush_buffer();
    }
}

void ProgressBar::suspend() {
    std::cout << "\033[2K\r" << std::flush;
}

void ProgressBar::resume() {
    if (m_impl) {
        m_impl->bar.print_progress();
        m_impl->flush_buffer();
    }
}

} // namespace foundation::infrastructure::progress
