module;
#include <string>

/**
 * @file core_utils_random.ixx
 * @brief Core utility module partition for random utilities
 * @author CodingRookie
 * @date 2026-01-18
 */
export module foundation.infrastructure.core_utils:random;

// import <string>;

namespace foundation::infrastructure::core_utils::random {

/**
 * @brief Generate a random string of specified length
 * @param length The length of the string to generate
 * @return Random string
 */
export std::string generate_random_str(size_t length);

/**
 * @brief Generate a UUID (Universally Unique Identifier)
 * @return UUID string
 */
export std::string generate_uuid();
} // namespace foundation::infrastructure::core_utils::random
