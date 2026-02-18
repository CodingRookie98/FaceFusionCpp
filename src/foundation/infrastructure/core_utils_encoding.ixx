module;
#include <string>

/**
 * @file core_utils_encoding.ixx
 * @brief Core utility module partition for encoding and locale utilities
 * @author CodingRookie
 * @date 2026-02-18
 */
export module foundation.infrastructure.core_utils:encoding;

namespace foundation::infrastructure::core_utils::encoding {

/**
 * @brief Set the global locale to UTF-8 (or system default on failure)
 */
export void set_global_locale_utf8();

#ifdef _WIN32
/**
 * @brief Convert UTF-8 string to System Default Local (ACP) for Windows console output
 * @param utf8_str The UTF-8 string to convert
 * @return String encoded in system default locale
 */
export std::string utf8_to_sys_default_local(const std::string& utf8_str);
#endif

} // namespace foundation::infrastructure::core_utils::encoding
