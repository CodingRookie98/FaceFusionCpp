/**
 ******************************************************************************
 * @file           : process.ixx
 * @brief          : Process management module interface (based on TinyProcessLib)
 ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

export module foundation.infrastructure.process;

export namespace foundation::infrastructure::process {

struct Config {
    std::size_t buffer_size = 131072;
    bool inherit_file_descriptors = false;
    std::function<void()> on_stdout_close = nullptr;
    std::function<void()> on_stderr_close = nullptr;

    enum class ShowWindow {
        hide = 0,
        show_normal = 1,
        show_minimized = 2,
        maximize = 3,
        show_maximized = 3,
        show_no_activate = 4,
        show = 5,
        minimize = 6,
        show_min_no_active = 7,
        show_na = 8,
        restore = 9,
        show_default = 10,
        force_minimize = 11
    };
    ShowWindow show_window{ShowWindow::show_default};
};

class Process {
public:
#ifdef _WIN32
    using id_type = unsigned long;
    using fd_type = void*;
    using string_type = std::string; // Keeping it simple with std::string (UTF-8)
#else
    using id_type = int;
    using fd_type = int;
    using string_type = std::string;
#endif
    using environment_type = std::unordered_map<string_type, string_type>;

    // Constructor with command string
    Process(const string_type& command, const string_type& path = string_type(),
            std::function<void(const char* bytes, size_t n)> read_stdout = nullptr,
            std::function<void(const char* bytes, size_t n)> read_stderr = nullptr,
            bool open_stdin = false,
            const Config& config = {}) noexcept;

    // Constructor with arguments vector
    Process(const std::vector<string_type>& arguments, const string_type& path = string_type(),
            std::function<void(const char* bytes, size_t n)> read_stdout = nullptr,
            std::function<void(const char* bytes, size_t n)> read_stderr = nullptr,
            bool open_stdin = false,
            const Config& config = {}) noexcept;

    ~Process() noexcept;

    id_type get_id() const noexcept;
    int get_exit_status() noexcept;
    bool try_get_exit_status(int& exit_status) noexcept;

    bool write(const char* bytes, size_t n);
    bool write(const std::string& str);
    void close_stdin() noexcept;

    void kill(bool force = false) noexcept;
    static void kill(id_type id, bool force = false) noexcept;

    // PIMPL idiom to hide platform details and avoiding including windows.h in module interface
    class Impl;

private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace foundation::infrastructure::process
