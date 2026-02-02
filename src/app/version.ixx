/**
 * @file version.ixx
 * @brief Application version information module
 * @note Version values are injected at compile time by CMake
 * @see design.md Section 5.5 - Metadata Management
 */
module;
#include <string>
#include <string_view>

export module app.version;

export namespace app::version {

/**
 * @brief Get the application name
 */
constexpr std::string_view app_name() noexcept;

/**
 * @brief Get the full version string (e.g., "0.33.0")
 */
constexpr std::string_view version() noexcept;

/**
 * @brief Get the major version number
 */
constexpr int version_major() noexcept;

/**
 * @brief Get the minor version number
 */
constexpr int version_minor() noexcept;

/**
 * @brief Get the patch version number
 */
constexpr int version_patch() noexcept;

/**
 * @brief Get the build timestamp (UTC)
 */
constexpr std::string_view build_time() noexcept;

/**
 * @brief Get the Git commit hash (short)
 */
constexpr std::string_view git_commit() noexcept;

/**
 * @brief Get the Git branch name
 */
constexpr std::string_view git_branch() noexcept;

/**
 * @brief Get formatted version banner string
 * @return Multi-line banner suitable for startup logging
 */
std::string get_banner();

/**
 * @brief Get single-line version string for --version output
 * @return e.g., "FaceFusionCpp v0.33.0 (abc1234)"
 */
std::string get_version_string();

} // namespace app::version
