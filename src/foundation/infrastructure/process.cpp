/**
 ******************************************************************************
 * @file           : process.cpp
 * @brief          : Process management module implementation (Cross-platform)
 ******************************************************************************
 */

module;

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <bitset>
#endif

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <format>
#include <algorithm>

module foundation.infrastructure.process;

namespace foundation::infrastructure::process {

#ifdef _WIN32
// =========================================================================
// Windows Implementation
// =========================================================================

namespace {
class Handle {
public:
    Handle() noexcept : handle(INVALID_HANDLE_VALUE) {}
    ~Handle() noexcept { close(); }
    void close() noexcept {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
    }
    HANDLE detach() noexcept {
        HANDLE old_handle = handle;
        handle = INVALID_HANDLE_VALUE;
        return old_handle;
    }
    operator HANDLE() const noexcept { return handle; }
    HANDLE* operator&() noexcept { return &handle; }

private:
    HANDLE handle;
};

template <class Char> struct CharSet;
template <> struct CharSet<char> {
    static constexpr const char* whitespace = " \t\n\v\"";
    static constexpr char space = ' ';
    static constexpr char doublequote = '"';
    static constexpr char backslash = '\\';
};

static std::string join_arguments(const std::vector<std::string>& args) {
    std::string ret;
    using charset = CharSet<char>;

    for (const auto& arg : args) {
        if (!ret.empty()) ret.push_back(charset::space);

        if (!arg.empty() && arg.find_first_of(charset::whitespace) == std::string::npos) {
            ret.append(arg);
            continue;
        }

        ret.push_back(charset::doublequote);
        for (auto it = arg.begin();; ++it) {
            size_t n_backslashes = 0;
            while (it != arg.end() && *it == charset::backslash) {
                ++it;
                ++n_backslashes;
            }

            if (it == arg.end()) {
                ret.append(n_backslashes * 2, charset::backslash);
                break;
            } else if (*it == charset::doublequote) {
                ret.append(n_backslashes * 2 + 1, charset::backslash);
                ret.push_back(*it);
            } else {
                ret.append(n_backslashes, charset::backslash);
                ret.push_back(*it);
            }
        }
        ret.push_back(charset::doublequote);
    }
    return ret;
}

std::mutex create_process_mutex;
} // namespace

class Process::Impl {
public:
    Impl() = default;
    unsigned long id{0};
    void* handle{nullptr};
    int exit_status{-1};

    bool closed{true};
    std::mutex close_mutex;

    std::function<void(const char*, size_t)> read_stdout;
    std::function<void(const char*, size_t)> read_stderr;

    std::thread stdout_thread, stderr_thread;
    bool open_stdin{false};
    std::mutex stdin_mutex;
    Config config;

    std::unique_ptr<void*> stdout_fd, stderr_fd, stdin_fd;

    void close_fds() noexcept {
        if (stdout_thread.joinable()) stdout_thread.join();
        if (stderr_thread.joinable()) stderr_thread.join();

        if (stdin_fd) close_stdin_fd();

        if (stdout_fd) {
            if (*stdout_fd != nullptr) CloseHandle(static_cast<HANDLE>(*stdout_fd));
            stdout_fd.reset();
        }
        if (stderr_fd) {
            if (*stderr_fd != nullptr) CloseHandle(static_cast<HANDLE>(*stderr_fd));
            stderr_fd.reset();
        }
    }

    void close_stdin_fd() noexcept {
        std::lock_guard<std::mutex> lock(stdin_mutex);
        if (stdin_fd) {
            if (*stdin_fd != nullptr) CloseHandle(static_cast<HANDLE>(*stdin_fd));
            stdin_fd.reset();
        }
    }
};

Process::Process(const string_type& command, const string_type& path,
                 std::function<void(const char*, size_t)> read_stdout,
                 std::function<void(const char*, size_t)> read_stderr, bool open_stdin,
                 const Config& config) noexcept : m_impl(std::make_unique<Impl>()) {
    m_impl->read_stdout = std::move(read_stdout);
    m_impl->read_stderr = std::move(read_stderr);
    m_impl->open_stdin = open_stdin;
    m_impl->config = config;

    if (m_impl->open_stdin) m_impl->stdin_fd = std::unique_ptr<void*>(new void*(nullptr));
    if (m_impl->read_stdout) m_impl->stdout_fd = std::unique_ptr<void*>(new void*(nullptr));
    if (m_impl->read_stderr) m_impl->stderr_fd = std::unique_ptr<void*>(new void*(nullptr));

    Handle stdin_rd_p, stdin_wr_p;
    Handle stdout_rd_p, stdout_wr_p;
    Handle stderr_rd_p, stderr_wr_p;

    SECURITY_ATTRIBUTES security_attributes;
    security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attributes.bInheritHandle = TRUE;
    security_attributes.lpSecurityDescriptor = nullptr;

    std::lock_guard<std::mutex> lock(create_process_mutex);

    bool success = true;
    if (m_impl->stdin_fd) {
        if (!CreatePipe(&stdin_rd_p, &stdin_wr_p, &security_attributes, 0)
            || !SetHandleInformation(stdin_wr_p, HANDLE_FLAG_INHERIT, 0))
            success = false;
    }
    if (success && m_impl->stdout_fd) {
        if (!CreatePipe(&stdout_rd_p, &stdout_wr_p, &security_attributes, 0)
            || !SetHandleInformation(stdout_rd_p, HANDLE_FLAG_INHERIT, 0))
            success = false;
    }
    if (success && m_impl->stderr_fd) {
        if (!CreatePipe(&stderr_rd_p, &stderr_wr_p, &security_attributes, 0)
            || !SetHandleInformation(stderr_rd_p, HANDLE_FLAG_INHERIT, 0))
            success = false;
    }

    if (!success) return;

    PROCESS_INFORMATION process_info;
    STARTUPINFOA startup_info;
    ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startup_info, sizeof(STARTUPINFOA));
    startup_info.cb = sizeof(STARTUPINFOA);
    startup_info.hStdInput = stdin_rd_p;
    startup_info.hStdOutput = stdout_wr_p;
    startup_info.hStdError = stderr_wr_p;

    if (m_impl->stdin_fd || m_impl->stdout_fd || m_impl->stderr_fd)
        startup_info.dwFlags |= STARTF_USESTDHANDLES;

    if (config.show_window != Config::ShowWindow::show_default) {
        startup_info.dwFlags |= STARTF_USESHOWWINDOW;
        startup_info.wShowWindow = static_cast<WORD>(config.show_window);
    }

    auto process_command = command;
    DWORD creation_flags =
        (m_impl->stdin_fd || m_impl->stdout_fd || m_impl->stderr_fd) ? CREATE_NO_WINDOW : 0;

    BOOL bSuccess =
        CreateProcessA(nullptr, process_command.empty() ? nullptr : &process_command[0], nullptr,
                       nullptr, TRUE, creation_flags, nullptr,
                       path.empty() ? nullptr : path.c_str(), &startup_info, &process_info);

    if (!bSuccess) return;

    CloseHandle(process_info.hThread);

    if (m_impl->stdin_fd) *m_impl->stdin_fd = stdin_wr_p.detach();
    if (m_impl->stdout_fd) *m_impl->stdout_fd = stdout_rd_p.detach();
    if (m_impl->stderr_fd) *m_impl->stderr_fd = stderr_rd_p.detach();

    m_impl->closed = false;
    m_impl->id = process_info.dwProcessId;
    m_impl->handle = process_info.hProcess;

    if (m_impl->id != 0) {
        if (m_impl->stdout_fd) {
            m_impl->stdout_thread = std::thread([this]() {
                DWORD n;
                std::unique_ptr<char[]> buffer(new char[m_impl->config.buffer_size]);
                for (;;) {
                    BOOL bRes =
                        ReadFile(static_cast<HANDLE>(*m_impl->stdout_fd), buffer.get(),
                                 static_cast<DWORD>(m_impl->config.buffer_size), &n, nullptr);
                    if (!bRes || n == 0) {
                        if (m_impl->config.on_stdout_close) m_impl->config.on_stdout_close();
                        break;
                    }
                    if (m_impl->read_stdout)
                        m_impl->read_stdout(buffer.get(), static_cast<size_t>(n));
                }
            });
        }
        if (m_impl->stderr_fd) {
            m_impl->stderr_thread = std::thread([this]() {
                DWORD n;
                std::unique_ptr<char[]> buffer(new char[m_impl->config.buffer_size]);
                for (;;) {
                    BOOL bRes =
                        ReadFile(static_cast<HANDLE>(*m_impl->stderr_fd), buffer.get(),
                                 static_cast<DWORD>(m_impl->config.buffer_size), &n, nullptr);
                    if (!bRes || n == 0) {
                        if (m_impl->config.on_stderr_close) m_impl->config.on_stderr_close();
                        break;
                    }
                    if (m_impl->read_stderr)
                        m_impl->read_stderr(buffer.get(), static_cast<size_t>(n));
                }
            });
        }
    }
}

Process::Process(const std::vector<string_type>& arguments, const string_type& path,
                 std::function<void(const char*, size_t)> read_stdout,
                 std::function<void(const char*, size_t)> read_stderr, bool open_stdin,
                 const Config& config) noexcept :
    Process(join_arguments(arguments), path, std::move(read_stdout), std::move(read_stderr),
            open_stdin, config) {}

Process::~Process() noexcept {
    m_impl->close_fds();
}

Process::id_type Process::get_id() const noexcept {
    return m_impl->id;
}

int Process::get_exit_status() noexcept {
    if (m_impl->id == 0) return -1;
    if (m_impl->handle == nullptr) return m_impl->exit_status;

    WaitForSingleObject(static_cast<HANDLE>(m_impl->handle), INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(static_cast<HANDLE>(m_impl->handle), &exit_code))
        m_impl->exit_status = -1;
    else m_impl->exit_status = static_cast<int>(exit_code);

    {
        std::lock_guard<std::mutex> lock(m_impl->close_mutex);
        CloseHandle(static_cast<HANDLE>(m_impl->handle));
        m_impl->handle = nullptr;
        m_impl->closed = true;
    }
    m_impl->close_fds();
    return m_impl->exit_status;
}

bool Process::try_get_exit_status(int& exit_status) noexcept {
    if (m_impl->id == 0) {
        exit_status = -1;
        return true;
    }
    if (m_impl->handle == nullptr) {
        exit_status = m_impl->exit_status;
        return true;
    }

    DWORD wait_status = WaitForSingleObject(static_cast<HANDLE>(m_impl->handle), 0);
    if (wait_status == WAIT_TIMEOUT) return false;

    DWORD tmp_exit;
    if (!GetExitCodeProcess(static_cast<HANDLE>(m_impl->handle), &tmp_exit)) exit_status = -1;
    else exit_status = static_cast<int>(tmp_exit);

    m_impl->exit_status = exit_status;

    {
        std::lock_guard<std::mutex> lock(m_impl->close_mutex);
        CloseHandle(static_cast<HANDLE>(m_impl->handle));
        m_impl->handle = nullptr;
        m_impl->closed = true;
    }
    m_impl->close_fds();
    return true;
}

bool Process::write(const char* bytes, size_t n) {
    if (!m_impl->open_stdin) throw std::invalid_argument("Stdin not opened");

    std::lock_guard<std::mutex> lock(m_impl->stdin_mutex);
    if (m_impl->stdin_fd) {
        DWORD written;
        BOOL bSuccess = WriteFile(static_cast<HANDLE>(*m_impl->stdin_fd), bytes,
                                  static_cast<DWORD>(n), &written, nullptr);
        return bSuccess && written > 0;
    }
    return false;
}

bool Process::write(const std::string& str) {
    return write(str.c_str(), str.size());
}

void Process::close_stdin() noexcept {
    m_impl->close_stdin_fd();
}

void Process::kill(bool force) noexcept {
    std::lock_guard<std::mutex> lock(m_impl->close_mutex);
    if (m_impl->id > 0 && !m_impl->closed) {
        TerminateProcess(static_cast<HANDLE>(m_impl->handle), 2);
    }
}

void Process::kill(id_type id, bool force) noexcept {
    HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, id);
    if (process_handle) {
        TerminateProcess(process_handle, 2);
        CloseHandle(process_handle);
    }
}

#else
// =========================================================================
// Unix/Linux Implementation
// =========================================================================

class Process::Impl {
public:
    Impl() = default;
    int id{-1};
    int exit_status{-1};

    bool closed{true};
    std::mutex close_mutex;

    std::function<void(const char*, size_t)> read_stdout;
    std::function<void(const char*, size_t)> read_stderr;

    std::thread stdout_stderr_thread;
    bool open_stdin{false};
    std::mutex stdin_mutex;
    Config config;

    std::unique_ptr<int> stdout_fd, stderr_fd, stdin_fd;

    void close_fds() noexcept {
        if (stdout_stderr_thread.joinable()) stdout_stderr_thread.join();

        if (stdin_fd) close_stdin_fd();

        if (stdout_fd) {
            if (id > 0) ::close(*stdout_fd);
            stdout_fd.reset();
        }
        if (stderr_fd) {
            if (id > 0) ::close(*stderr_fd);
            stderr_fd.reset();
        }
    }

    void close_stdin_fd() noexcept {
        std::lock_guard<std::mutex> lock(stdin_mutex);
        if (stdin_fd) {
            if (id > 0) ::close(*stdin_fd);
            stdin_fd.reset();
        }
    }
};

Process::Process(const string_type& command, const string_type& path,
                 std::function<void(const char*, size_t)> read_stdout_func,
                 std::function<void(const char*, size_t)> read_stderr_func, bool open_stdin_flag,
                 const Config& config) noexcept : m_impl(std::make_unique<Impl>()) {
    m_impl->read_stdout = std::move(read_stdout_func);
    m_impl->read_stderr = std::move(read_stderr_func);
    m_impl->open_stdin = open_stdin_flag;
    m_impl->config = config;

    if (m_impl->open_stdin) m_impl->stdin_fd = std::make_unique<int>();
    if (m_impl->read_stdout) m_impl->stdout_fd = std::make_unique<int>();
    if (m_impl->read_stderr) m_impl->stderr_fd = std::make_unique<int>();

    int stdin_p[2], stdout_p[2], stderr_p[2];

    if (m_impl->stdin_fd && pipe(stdin_p) != 0) return;
    if (m_impl->stdout_fd && pipe(stdout_p) != 0) {
        if (m_impl->stdin_fd) {
            ::close(stdin_p[0]);
            ::close(stdin_p[1]);
        }
        return;
    }
    if (m_impl->stderr_fd && pipe(stderr_p) != 0) {
        if (m_impl->stdin_fd) {
            ::close(stdin_p[0]);
            ::close(stdin_p[1]);
        }
        if (m_impl->stdout_fd) {
            ::close(stdout_p[0]);
            ::close(stdout_p[1]);
        }
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        if (m_impl->stdin_fd) {
            ::close(stdin_p[0]);
            ::close(stdin_p[1]);
        }
        if (m_impl->stdout_fd) {
            ::close(stdout_p[0]);
            ::close(stdout_p[1]);
        }
        if (m_impl->stderr_fd) {
            ::close(stderr_p[0]);
            ::close(stderr_p[1]);
        }
        return;
    } else if (pid == 0) {
        // Child process
        if (m_impl->stdin_fd) dup2(stdin_p[0], 0);
        if (m_impl->stdout_fd) dup2(stdout_p[1], 1);
        if (m_impl->stderr_fd) dup2(stderr_p[1], 2);

        if (m_impl->stdin_fd) {
            ::close(stdin_p[0]);
            ::close(stdin_p[1]);
        }
        if (m_impl->stdout_fd) {
            ::close(stdout_p[0]);
            ::close(stdout_p[1]);
        }
        if (m_impl->stderr_fd) {
            ::close(stderr_p[0]);
            ::close(stderr_p[1]);
        }

        if (!config.inherit_file_descriptors) {
            int fd_max = std::min(8192, static_cast<int>(sysconf(_SC_OPEN_MAX)));
            if (fd_max < 0) fd_max = 8192;
            for (int fd = 3; fd < fd_max; fd++) ::close(fd);
        }

        setpgid(0, 0);

        std::string cd_path_and_command;
        const char* command_c_str = command.c_str();
        if (!path.empty()) {
            auto path_escaped = path;
            size_t pos = 0;
            while ((pos = path_escaped.find('\'', pos)) != std::string::npos) {
                path_escaped.replace(pos, 1, "'\\''");
                pos += 4;
            }
            cd_path_and_command = "cd '" + path_escaped + "' && " + command;
            command_c_str = cd_path_and_command.c_str();
        }

        execl("/bin/sh", "/bin/sh", "-c", command_c_str, nullptr);
        _exit(EXIT_FAILURE);
    }

    // Parent process
    if (m_impl->stdin_fd) ::close(stdin_p[0]);
    if (m_impl->stdout_fd) ::close(stdout_p[1]);
    if (m_impl->stderr_fd) ::close(stderr_p[1]);

    if (m_impl->stdin_fd) *m_impl->stdin_fd = stdin_p[1];
    if (m_impl->stdout_fd) *m_impl->stdout_fd = stdout_p[0];
    if (m_impl->stderr_fd) *m_impl->stderr_fd = stderr_p[0];

    m_impl->closed = false;
    m_impl->id = pid;

    // Async read
    if (m_impl->id > 0 && (m_impl->stdout_fd || m_impl->stderr_fd)) {
        m_impl->stdout_stderr_thread = std::thread([this]() {
            std::vector<pollfd> pollfds;
            std::bitset<2> fd_is_stdout;

            if (m_impl->stdout_fd) {
                fd_is_stdout.set(pollfds.size());
                pollfds.emplace_back();
                pollfds.back().fd = fcntl(*m_impl->stdout_fd, F_SETFL,
                                          fcntl(*m_impl->stdout_fd, F_GETFL) | O_NONBLOCK)
                                         == 0 ?
                                      *m_impl->stdout_fd :
                                      -1;
                pollfds.back().events = POLLIN;
            }
            if (m_impl->stderr_fd) {
                pollfds.emplace_back();
                pollfds.back().fd = fcntl(*m_impl->stderr_fd, F_SETFL,
                                          fcntl(*m_impl->stderr_fd, F_GETFL) | O_NONBLOCK)
                                         == 0 ?
                                      *m_impl->stderr_fd :
                                      -1;
                pollfds.back().events = POLLIN;
            }

            auto buffer = std::unique_ptr<char[]>(new char[m_impl->config.buffer_size]);
            bool any_open = !pollfds.empty();

            while (any_open
                   && (poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), -1) > 0
                       || errno == EINTR)) {
                any_open = false;
                for (size_t i = 0; i < pollfds.size(); ++i) {
                    if (pollfds[i].fd >= 0) {
                        if (pollfds[i].revents & POLLIN) {
                            const ssize_t n =
                                read(pollfds[i].fd, buffer.get(), m_impl->config.buffer_size);
                            if (n > 0) {
                                if (fd_is_stdout[i]) {
                                    if (m_impl->read_stdout)
                                        m_impl->read_stdout(buffer.get(), static_cast<size_t>(n));
                                } else {
                                    if (m_impl->read_stderr)
                                        m_impl->read_stderr(buffer.get(), static_cast<size_t>(n));
                                }
                            } else if (n == 0
                                       || (n < 0 && errno != EINTR && errno != EAGAIN
                                           && errno != EWOULDBLOCK)) {
                                if (fd_is_stdout[i]) {
                                    if (m_impl->config.on_stdout_close)
                                        m_impl->config.on_stdout_close();
                                } else {
                                    if (m_impl->config.on_stderr_close)
                                        m_impl->config.on_stderr_close();
                                }
                                pollfds[i].fd = -1;
                                continue;
                            }
                        } else if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                            if (fd_is_stdout[i]) {
                                if (m_impl->config.on_stdout_close)
                                    m_impl->config.on_stdout_close();
                            } else {
                                if (m_impl->config.on_stderr_close)
                                    m_impl->config.on_stderr_close();
                            }
                            pollfds[i].fd = -1;
                            continue;
                        }
                        any_open = true;
                    }
                }
            }
        });
    }
}

Process::Process(const std::vector<string_type>& arguments, const string_type& path,
                 std::function<void(const char*, size_t)> read_stdout,
                 std::function<void(const char*, size_t)> read_stderr, bool open_stdin,
                 const Config& config) noexcept : m_impl(std::make_unique<Impl>()) {
    // Convert arguments to a command string for simplicity
    std::string command;
    for (const auto& arg : arguments) {
        if (!command.empty()) command += ' ';
        // Simple quoting for arguments with spaces
        if (arg.find(' ') != std::string::npos || arg.find('\'') != std::string::npos) {
            command += '\'';
            for (char c : arg) {
                if (c == '\'') command += "'\\''";
                else command += c;
            }
            command += '\'';
        } else {
            command += arg;
        }
    }
    // Delegate to string command constructor via placement new
    this->~Process();
    new (this)
        Process(command, path, std::move(read_stdout), std::move(read_stderr), open_stdin, config);
}

Process::~Process() noexcept {
    m_impl->close_fds();
}

Process::id_type Process::get_id() const noexcept {
    return m_impl->id;
}

int Process::get_exit_status() noexcept {
    if (m_impl->id <= 0) return -1;

    int exit_status;
    pid_t pid;
    do { pid = waitpid(m_impl->id, &exit_status, 0); } while (pid < 0 && errno == EINTR);

    if (pid < 0 && errno == ECHILD) {
        return m_impl->exit_status;
    } else {
        if (exit_status >= 256) exit_status = exit_status >> 8;
        m_impl->exit_status = exit_status;
    }

    {
        std::lock_guard<std::mutex> lock(m_impl->close_mutex);
        m_impl->closed = true;
    }
    m_impl->close_fds();
    return m_impl->exit_status;
}

bool Process::try_get_exit_status(int& exit_status) noexcept {
    if (m_impl->id <= 0) {
        exit_status = -1;
        return true;
    }

    const pid_t pid = waitpid(m_impl->id, &exit_status, WNOHANG);
    if (pid < 0 && errno == ECHILD) {
        exit_status = m_impl->exit_status;
        return true;
    } else if (pid <= 0) {
        return false;
    } else {
        if (exit_status >= 256) exit_status = exit_status >> 8;
        m_impl->exit_status = exit_status;
    }

    {
        std::lock_guard<std::mutex> lock(m_impl->close_mutex);
        m_impl->closed = true;
    }
    m_impl->close_fds();
    return true;
}

bool Process::write(const char* bytes, size_t n) {
    if (!m_impl->open_stdin) throw std::invalid_argument("Stdin not opened");

    std::lock_guard<std::mutex> lock(m_impl->stdin_mutex);
    if (m_impl->stdin_fd) {
        while (n != 0) {
            const ssize_t ret = ::write(*m_impl->stdin_fd, bytes, n);
            if (ret < 0) {
                if (errno == EINTR) continue;
                else return false;
            }
            bytes += static_cast<size_t>(ret);
            n -= static_cast<size_t>(ret);
        }
        return true;
    }
    return false;
}

bool Process::write(const std::string& str) {
    return write(str.c_str(), str.size());
}

void Process::close_stdin() noexcept {
    m_impl->close_stdin_fd();
}

void Process::kill(bool force) noexcept {
    std::lock_guard<std::mutex> lock(m_impl->close_mutex);
    if (m_impl->id > 0 && !m_impl->closed) {
        if (force) {
            ::kill(-m_impl->id, SIGTERM);
            ::kill(m_impl->id, SIGTERM);
        } else {
            ::kill(-m_impl->id, SIGINT);
            ::kill(m_impl->id, SIGINT);
        }
    }
}

void Process::kill(id_type id, bool force) noexcept {
    if (id <= 0) return;
    if (force) {
        ::kill(-id, SIGTERM);
        ::kill(id, SIGTERM);
    } else {
        ::kill(-id, SIGINT);
        ::kill(id, SIGINT);
    }
}

#endif // _WIN32

} // namespace foundation::infrastructure::process
