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

using namespace ffc;

int main() {
    FileSystem::setLocalToUTF8();

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << std::format("{} v{} {} By {}", metadata::name, metadata::version, metadata::url, metadata::author) << std::endl;
    std::cout << std::format("onnxruntime v{}", Ort::GetVersionString()) << std::endl;
    std::cout << std::format("OpenCV v{}", cv::getVersionString()) << std::endl;

    const std::string tmpPath = FileSystem::getTempPath() + "/" + metadata::name;
    if (FileSystem::dirExists(tmpPath)) {
        FileSystem::removeDir(tmpPath);
    }

    ini_config ini_config;
    ini_config.loadConfig();

    const auto core = std::make_shared<ffc::Core>(ini_config.getCoreOptions());
    const bool ok = core->Run(ini_config.getCoreRunOptions());

    if (!ok) {
        Logger::getInstance()->error("FaceFusionCpp failed to run. Maybe some of the tasks failed.");
    }
    if (FileSystem::dirExists(tmpPath)) {
        FileSystem::removeDir(tmpPath);
    }
    return 0;
}
