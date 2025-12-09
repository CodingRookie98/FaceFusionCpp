#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

import processor_hub;
import core;
import ini_config;
import logger;
import metadata;
import file_system;
import cli_config;

using namespace ffc;

int main(int argc, char** argv) {
    FileSystem::setLocalToUTF8();

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << std::format("{} v{} {} By {}", metadata::name, metadata::version, metadata::url, metadata::author) << std::endl;
    std::cout << std::format("onnxruntime v{}", Ort::GetVersionString()) << std::endl;
    std::cout << std::format("OpenCV v{}", cv::getVersionString()) << std::endl;

    const std::string tmpPath = FileSystem::getTempPath() + "/" + metadata::name;
    FileSystem::removeDir(tmpPath);

    ini_config ini_config;
    ini_config.loadConfig();

    auto coreOptions = ini_config.getCoreOptions();
    auto coreTask = ini_config.getCoreRunOptions();

    CliConfig cliConfig;
    if (!cliConfig.parse(argc, argv, coreOptions, coreTask)) {
        return 0;
    }

    const auto core = std::make_shared<ffc::Core>(coreOptions);
    const bool ok = core->Run(coreTask);

    if (!ok) {
        Logger::getInstance()->error("FaceFusionCpp failed to run. Maybe some of the tasks failed.");
    }
    FileSystem::removeDir(tmpPath);
    return 0;
}
