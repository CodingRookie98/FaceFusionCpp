module;
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

/**
 * @file process.ixx
 * @brief Process management module based on TinyProcessLib
 * @author
 * CodingRookie
 * @date 2026-01-27
 */
export module foundation.infrastructure.process;

export namespace foundation::infrastructure::process {

/**
 * @brief Configuration for process execution
 */
struct Config {
    std::size_t buffer_size = 131072;
    bool inherit_file_descriptors = false;
    std::function<void()> on_stdout_close = nullptr;
    std::function<void()> on_stderr_close = nullptr;

    /**
     * @brief Window display mode
     */
    enum class ShowWindow {
        hide = 0,               ///< Hide the window
        show_normal = 1,        ///< Activate and display in a normal size
        show_minimized = 2,     ///< Activate and display minimized
        maximize = 3,           ///< Activate and display maximized
        show_maximized = 3,     ///< Alias for maximize
        show_no_activate = 4,   ///< Display in its most recent size and position, do not activate
        show = 5,               ///< Activate and show
        minimize = 6,           ///< Minimize
        show_min_no_active = 7, ///< Display minimized, do not activate
        show_na = 8,            ///< Display in current state, do not activate
        restore = 9,            ///< Activate and display (restore if minimized/maximized)
        show_default = 10,      ///< Sets the show state based on the STARTUPINFO
        force_minimize =
            11 ///< Minimizes a window, even if the thread that owns the window is not responding
    };
    ShowWindow show_window{ShowWindow::show_default};
};

/**
 * @brief Process class for spawning and controlling child processes
 */
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

    /**
     * @brief Constructor with command string
     * @param command Command to execute
     * @param path Working directory
     * @param read_stdout Callback for stdout
     * @param read_stderr Callback for stderr
     * @param open_stdin Whether to open stdin
     * @param config Process configuration
     */
    Process(const string_type& command, const string_type& path = string_type(),
            std::function<void(const char* bytes, size_t n)> read_stdout = nullptr,
            std::function<void(const char* bytes, size_t n)> read_stderr = nullptr,
            bool open_stdin = false, const Config& config = {}) noexcept;

    /**
     * @brief Constructor with arguments vector
     * @param arguments List of arguments (first is executable)
     * @param path Working directory
     * @param read_stdout Callback for stdout
     * @param read_stderr Callback for stderr
     * @param open_stdin Whether to open stdin
     * @param config Process configuration
     */
    Process(const std::vector<string_type>& arguments, const string_type& path = string_type(),
            std::function<void(const char* bytes, size_t n)> read_stdout = nullptr,
            std::function<void(const char* bytes, size_t n)> read_stderr = nullptr,
            bool open_stdin = false, const Config& config = {}) noexcept;

    ~Process() noexcept;

    /**
     * @brief Get process ID
     * @return Process ID
     */
    id_type get_id() const noexcept;

    /**
     * @brief Get exit status (blocking)
     * @return Exit status code
     */
    int get_exit_status() noexcept;

    /**
     * @brief Try to get exit status (non-blocking)
     * @param exit_status Output parameter for exit status
     * @return True if process has exited, false otherwise
     */
    bool try_get_exit_status(int& exit_status) noexcept;

    /**
     * @brief Write to stdin
     * @param bytes Data buffer
     * @param n Size of data
     * @return True if successful
     */
    bool write(const char* bytes, size_t n);

    /**
     * @brief Write string to stdin
     * @param str String to write
     * @return True if successful
     */
    bool write(const std::string& str);

    /**
     * @brief Close stdin
     */
    void close_stdin() noexcept;

    /**
     * @brief Kill the process
     * @param force Force kill
     */
    void kill(bool force = false) noexcept;

    /**
     * @brief Static helper to kill a process by ID
     * @param id Process ID
     * @param force Force kill
     */
    static void kill(id_type id, bool force = false) noexcept;

    // PIMPL idiom to hide platform details and avoiding including windows.h in module interface
    class Impl;

private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace foundation::infrastructure::process
