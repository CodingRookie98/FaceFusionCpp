/**
 ******************************************************************************
 * @file           : progress_m_bar->cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-28
 ******************************************************************************
 */

module;
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>

module progress_bar;

namespace ffc {
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

void ProgressBar::setMaxProgress(const int64_t &max) const {
    int64_t setMax = max;
    if (max < 0) {
        setMax = 0;
    } else if (max > 100) {
        setMax = 100;
    }
    m_bar->set_option(option::MaxProgress{setMax});
}

void ProgressBar::setPrefixText(const std::string &text) const {
    m_bar->set_option(option::PrefixText{text});
}

void ProgressBar::setPostfixText(const std::string &text) const {
    m_bar->set_option(option::PostfixText{text});
}

void ProgressBar::setProgress(const unsigned int &progress) const {
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

void ProgressBar::markAsCompleted() const {
    m_bar->mark_as_completed();
}

void ProgressBar::showConsoleCursor(const bool &show) {
    show_console_cursor(show);
}

bool ProgressBar::isCompleted() const {
    return m_bar->is_completed();
}
} // namespace ffc