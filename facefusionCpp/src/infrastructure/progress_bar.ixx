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

    void setMaxProgress(const int64_t& max) const;
    void setPrefixText(const std::string& text) const;
    void setPostfixText(const std::string& text) const;
    void setProgress(const unsigned int& progress) const;
    void tick() const;
    void markAsCompleted() const;
    static void showConsoleCursor(const bool& show);
    [[nodiscard]] bool isCompleted() const;

private:
    indicators::ProgressBar* m_bar;
};

} // namespace ffc::infra
