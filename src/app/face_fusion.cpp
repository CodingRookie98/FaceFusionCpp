/**
 * @file face_fusion.cpp
 * @brief Application entry point
 * @author CodingRookie
 * @date 2026-01-18
 */
import app.cli;
import foundation.infrastructure.core_utils;

/**
 * @brief Main function
 * @return Exit code
 */
int main(int argc, char** argv) {
#ifdef _WIN32
    foundation::infrastructure::core_utils::encoding::set_global_locale_utf8();
#endif
    return app::cli::App::run(argc, argv);
}
