#include <iostream>
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

    std::cout << std::format("{} {} {} By {}", metadata::name, metadata::version, metadata::url, metadata::author) << std::endl;
    std::cout << std::format("onnxruntime {}", Ort::GetVersionString()) << std::endl;
    std::cout << std::format("OpenCV {}", cv::getVersionString()) << std::endl;

    ini_config ini_config;
    ini_config.loadConfig();
    const auto core = std::make_shared<ffc::Core>(ini_config.getCoreOptions());
    const bool ok = core->run(ini_config.getCoreRunOptions());

    return 0;
}
