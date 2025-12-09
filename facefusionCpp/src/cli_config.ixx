/**
 * @file           : cli_config.ixx
 * @brief          : CLI Configuration Loader
 */

module;
#include <CLI/CLI.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

export module cli_config;
import core;
import core_options;
import file_system;
import logger;
import metadata;

namespace ffc {

export class CliConfig {
public:
    CliConfig() = default;

    /**
     * @brief Parses command line arguments and updates options and task.
     * @param argc Argument count
     * @param argv Argument values
     * @param coreOptions Options to update
     * @param coreTask Task to update
     * @return true if execution should continue, false if help was shown or error occurred
     */
    bool parse(int argc, char** argv, CoreOptions& coreOptions, CoreTask& coreTask) {
        CLI::App app{"FaceFusionCpp - Next generation face swapper and enhancer"};
        app.set_help_all_flag("--help-all", "Show all help");

        bool show_version = false;
        app.add_flag("-v,--version", show_version, "Show version information");

        // General
        bool headless = false;
        app.add_flag("--headless", headless, "Run in headless mode (console only)");

        // Paths
        std::vector<std::string> source_paths;
        std::vector<std::string> target_paths;
        std::string output_path;

        app.add_option("-s,--source", source_paths, "Source paths (file, directory, or pattern)");
        app.add_option("-t,--target", target_paths, "Target paths (file, directory, or pattern)");
        app.add_option("-o,--output", output_path, "Output directory path");

        // Execution
        app.add_option("--threads", coreOptions.execution_thread_count, "Execution thread count");

        // Execution Device (basic support)
        // If we want to support device selection via CLI, we verify against available providers
        // For now, keeping it simple or maybe mapping a string to provider?
        // coreOptions.inference_session_options.execution_providers is a set.
        // It's complex to map cleanly without logic. I'll skip complex types for now in this iteration.

        try {
            app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            app.exit(e);
            return false;
        }

        // Apply Headless (not currently used in CoreOptions explicitly but can be logic in main)
        // If headless, we might want to suppress some GUIs if they existed, but currently it's console app mostly.
        // However, CoreTask has `show_progress_bar`.
        if (show_version) {
            std::cout << std::format("{} v{} {} By {}", metadata::name, metadata::version, metadata::url, metadata::author) << std::endl;
            std::cout << std::format("onnxruntime v{}", Ort::GetVersionString()) << std::endl;
            std::cout << std::format("OpenCV v{}", cv::getVersionString()) << std::endl;
            return false;
        }

        if (headless) {
            // Maybe disable progress bar? Or just ensure no windows pop up (Core::ProcessImages uses cv::imshow if debug? No.)
            // CoreTask has show_progress_bar defaults to true.
            coreTask.show_progress_bar = false;
        }

        if (!source_paths.empty()) {
             coreTask.source_paths = source_paths; // Directly assign for now. Logic for dir scanning is in core or ini?
             // ini_config scans directories. CLI usually expects user to provide exact files OR we should replicate scanning logic?
             // CoreTask expects list of FILES usually?
             // ini_config::paths() does scanning:
             /*
                if (FileSystem::isDir(value)) {
                    std::unordered_set<std::string> filePaths = FileSystem::listFilesInDir(value);
                    ...
                }
             */
             // I should probably iterate and expand directories here to match ini_config behavior.
             std::vector<std::string> expanded_source;
             for(const auto& p : source_paths) {
                 if (FileSystem::isDir(p)) {
                     auto files = FileSystem::listFilesInDir(p);
                     expanded_source.insert(expanded_source.end(), files.begin(), files.end());
                 } else {
                     expanded_source.push_back(p);
                 }
             }
             coreTask.source_paths = expanded_source;
        }

        if (!target_paths.empty()) {
             std::vector<std::string> expanded_target;
             for(const auto& p : target_paths) {
                 if (FileSystem::isDir(p)) {
                     auto files = FileSystem::listFilesInDir(p);
                     expanded_target.insert(expanded_target.end(), files.begin(), files.end());
                 } else {
                     expanded_target.push_back(p);
                 }
             }
             coreTask.target_paths = expanded_target;
        }

        if (!output_path.empty()) {
             coreTask.output_paths = FileSystem::normalizeOutputPaths(coreTask.target_paths, output_path);
        } else if (!target_paths.empty()) {
             // If target paths changed but output not specified, check if we can rely on default
             // Default output in ini_config is "./output".
             coreTask.output_paths = FileSystem::normalizeOutputPaths(coreTask.target_paths, "./output");
        }

        return true;
    }
};

} // namespace ffc
