
module;
#include <filesystem>
module foundation.infrastructure.test_support;

namespace foundation::infrastructure::test {
void reset_environment() {
    // Example: cleanup temp files used in tests
    if (std::filesystem::exists("test_temp")) { std::filesystem::remove_all("test_temp"); }
}
} // namespace foundation::infrastructure::test
