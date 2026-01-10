module;
#include <string>

export module foundation.infrastructure.core_utils:random;

// import <string>;

namespace foundation::infrastructure::core_utils::random {
export std::string generate_random_str(size_t length);
export std::string generate_uuid();
} // namespace foundation::infrastructure::core_utils::random
