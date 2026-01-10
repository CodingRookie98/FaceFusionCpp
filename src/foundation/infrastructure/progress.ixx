module;
#include <string>
#include <memory>

export module foundation.infrastructure.progress;

// import <string>;
// import <memory>;

export namespace foundation::infrastructure::progress {

struct IProgressObserver {
    virtual ~IProgressObserver()                               = default;
    virtual void on_progress(int step, const std::string& msg) = 0;
};

class ProgressBar {
public:
    ProgressBar();
    ~ProgressBar(); // Required for PIMPL

    // Delete copy/move as it holds unique_ptr to external resource potentially
    ProgressBar(const ProgressBar&)            = delete;
    ProgressBar& operator=(const ProgressBar&) = delete;
    ProgressBar(ProgressBar&&);
    ProgressBar& operator=(ProgressBar&&);

    void set_progress(float percent);
    void set_postfix_text(const std::string& text);
    void tick();
    bool is_completed() const;
    void mark_as_completed();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace foundation::infrastructure::progress
