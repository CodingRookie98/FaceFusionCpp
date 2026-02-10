module;
#include <filesystem>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iostream>
export module tests.helpers.foundation.test_utilities;

namespace fs = std::filesystem;

export namespace tests::helpers::foundation {
void reset_environment() {
    // Example: cleanup temp files used in tests
    if (std::filesystem::exists("test_temp")) { std::filesystem::remove_all("test_temp"); }
}

std::filesystem::path get_assets_path() {
    // 1. Check Env Var
    if (const char* env_p = std::getenv("FACEFUSION_ASSETS_PATH")) {
        fs::path path(env_p);
        if (fs::exists(path) && fs::is_directory(path)) { return fs::absolute(path); }
    }

    // 2. Search upwards
    fs::path current = fs::current_path();
    // Safety break to prevent infinite loops (though root check should handle it)
    int max_depth = 10;

    while (max_depth-- > 0) {
        fs::path potential = current / "assets";
        // Check if it's a directory AND contains models_info.json to ensure it's the real assets
        if (fs::exists(potential) && fs::is_directory(potential)
            && fs::exists(potential / "models_info.json")) {
            return fs::absolute(potential);
        }

        if (!current.has_parent_path() || current == current.parent_path()) { break; }
        current = current.parent_path();
    }

    // Fallback: try to print where we looked (optional, but good for debugging)
    // For now, throw.
    throw std::runtime_error("Could not find assets directory. Checked upwards from: "
                             + fs::current_path().string());
}

std::filesystem::path get_test_data_path(const std::string& relative_path) {
    return get_assets_path() / relative_path;
}

} // namespace tests::helpers::foundation
