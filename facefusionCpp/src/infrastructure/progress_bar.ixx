/**
 ******************************************************************************
 * @file           : progress_bar.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-28
 ******************************************************************************
 */

module;
#include <indicators/progress_bar.hpp>

export module progress_bar;

using namespace indicators;

export namespace ffc::infra {

class ProgressBar {
public:
    ProgressBar();
    ~ProgressBar();

    void set_max_progress(const int64_t& max) const;
    void set_prefix_text(const std::string& text) const;
    void set_postfix_text(const std::string& text) const;
    void set_progress(const unsigned int& progress) const;
    void tick() const;
    void mark_as_completed() const;
    static void show_console_cursor(const bool& show);
    [[nodiscard]] bool is_completed() const;

private:
    indicators::ProgressBar* m_bar;
};

} // namespace ffc::infra
