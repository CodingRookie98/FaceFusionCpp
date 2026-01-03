/**
 * @file progress_bar.ixx
 * @brief Progress bar module for displaying progress information
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides progress bar functionality using indicators library
 */

module;
#include <indicators/progress_bar.hpp>

export module progress_bar;

using namespace indicators;

export namespace ffc::infra {

/**
 * @brief Progress bar class for displaying task progress
 * @note This class provides a progress bar using indicators library for visual progress tracking
 */
class ProgressBar {
public:
    /**
     * @brief Construct a progress bar instance
     * @note Initializes the progress bar with default settings
     */
    ProgressBar();

    /**
     * @brief Destroy the progress bar instance
     */
    ~ProgressBar();

    /**
     * @brief Set the maximum progress value
     * @param max Maximum progress value
     * @note This value represents 100% completion
     */
    void set_max_progress(const int64_t& max) const;

    /**
     * @brief Set the prefix text displayed before the progress bar
     * @param text Prefix text to display
     */
    void set_prefix_text(const std::string& text) const;

    /**
     * @brief Set the postfix text displayed after the progress bar
     * @param text Postfix text to display
     */
    void set_postfix_text(const std::string& text) const;

    /**
     * @brief Set the current progress value
     * @param progress Current progress value
     * @note The progress value should be between 0 and max_progress
     */
    void set_progress(const unsigned int& progress) const;

    /**
     * @brief Increment the progress by one step
     * @note This is a convenience method to increment progress by 1
     */
    void tick() const;

    /**
     * @brief Mark the progress bar as completed
     * @note Sets progress to 100% and displays completion status
     */
    void mark_as_completed() const;

    /**
     * @brief Show or hide the console cursor
     * @param show True to show cursor, false to hide
     * @note This is a static method that affects the entire console
     */
    static void show_console_cursor(const bool& show);

    /**
     * @brief Check if the progress bar is completed
     * @return bool True if completed, false otherwise
     */
    [[nodiscard]] bool is_completed() const;

private:
    indicators::ProgressBar* m_bar; ///< Pointer to indicators::ProgressBar instance
};

} // namespace ffc::infra
