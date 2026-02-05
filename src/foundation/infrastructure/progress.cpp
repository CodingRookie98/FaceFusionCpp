
module;

#include <cstdint>
#include <indicators/progress_bar.hpp>
#include <string>
#include <memory>
#include <vector>

module foundation.infrastructure.progress;

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
    m_impl(std::make_unique<Impl>(postfix_text)) {}
ProgressBar::~ProgressBar() = default;

ProgressBar::ProgressBar(ProgressBar&&) = default;
ProgressBar& ProgressBar::operator=(ProgressBar&&) = default;

void ProgressBar::set_progress(float percent) {
    if (m_impl) m_impl->bar.set_progress(percent);
}

void ProgressBar::set_postfix_text(const std::string& text) {
    if (m_impl) m_impl->bar.set_option(indicators::option::PostfixText{text});
}

void ProgressBar::tick() {
    if (m_impl) m_impl->bar.tick();
}

bool ProgressBar::is_completed() const {
    if (m_impl) return m_impl->bar.is_completed();
    return false;
}

void ProgressBar::mark_as_completed() {
    if (m_impl) m_impl->bar.mark_as_completed();
}
} // namespace foundation::infrastructure::progress
