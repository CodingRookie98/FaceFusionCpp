module;
#include <string>
#include <memory>

/**
 * @file progress.ixx
 * @brief Progress tracking module (based on indicators)
 * @author
 * CodingRookie
 * @date 2026-01-27
 */
export module foundation.infrastructure.progress;

export namespace foundation::infrastructure::progress {

/**
 * @brief Interface for progress observation
 */
struct IProgressObserver {
    virtual ~IProgressObserver() = default;
    /**
     * @brief Callback for progress update
     * @param step Current step or percentage
     * @param msg Progress message
     */
    virtual void on_progress(int step, const std::string& msg) = 0;
};

/**
 * @brief Console progress bar implementation
 */
class ProgressBar {
public:
    ProgressBar();
    ~ProgressBar(); // Required for PIMPL

    // Delete copy/move as it holds unique_ptr to external resource potentially
    ProgressBar(const ProgressBar&) = delete;
    ProgressBar& operator=(const ProgressBar&) = delete;
    ProgressBar(ProgressBar&&);
    ProgressBar& operator=(ProgressBar&&);

    /**
     * @brief Set current progress percentage
     * @param percent Progress (0.0 - 100.0)
     */
    void set_progress(float percent);

    /**
     * @brief Set text to display after the bar
     * @param text Postfix text
     */
    void set_postfix_text(const std::string& text);

    /**
     * @brief Advance progress by one tick
     */
    void tick();

    /**
     * @brief Check if progress is completed
     * @return True if completed
     */
    bool is_completed() const;

    /**
     * @brief Mark progress as completed
     */
    void mark_as_completed();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace foundation::infrastructure::progress
