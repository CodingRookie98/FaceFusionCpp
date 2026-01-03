/**
 * @file progress_bar.cpp
 * @brief Progress bar module implementation
 * @author CodingRookie
 * @date 2026-01-04
 * @note This file contains the implementation of the progress bar module
 */

module;
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>

module progress_bar;

namespace ffc::infra {
ProgressBar::ProgressBar() {
    m_bar = new indicators::ProgressBar{
        option::BarWidth{50},
        option::MaxProgress{static_cast<int64_t>(0)},
        option::Start{" ["},
        option::Fill{"="},
        option::Lead{">"},
        option::Remainder{" "},
        option::End{"]"},
        option::PrefixText{""},
        option::ForegroundColor{Color::green},
        option::ShowElapsedTime{true},
        option::ShowRemainingTime{true},
        option::ShowPercentage{true},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}};
}

ProgressBar::~ProgressBar() {
    delete m_bar;
}

void ProgressBar::set_max_progress(const int64_t& max) const {
    int64_t setMax = max;
    if (max < 0) {
        setMax = 0;
    } else if (max > 100) {
        setMax = 100;
    }
    m_bar->set_option(option::MaxProgress{setMax});
}

void ProgressBar::set_prefix_text(const std::string& text) const {
    m_bar->set_option(option::PrefixText{text});
}

void ProgressBar::set_postfix_text(const std::string& text) const {
    m_bar->set_option(option::PostfixText{text});
}

void ProgressBar::set_progress(const unsigned int& progress) const {
    unsigned int setProgress = progress;
    if (progress < 0) {
        setProgress = 0;
    }
    if (progress > 100) {
        setProgress = 100;
    }
    m_bar->set_progress(setProgress);
}

void ProgressBar::tick() const {
    m_bar->tick();
}

void ProgressBar::mark_as_completed() const {
    m_bar->mark_as_completed();
}

void ProgressBar::show_console_cursor(const bool& show) {
    show_console_cursor(show);
}

bool ProgressBar::is_completed() const {
    return m_bar->is_completed();
}
} // namespace ffc::infra