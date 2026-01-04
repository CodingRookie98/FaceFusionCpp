#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <fstream>

import processor_hub;
import core;
import core_options;
import logger;
import metadata;
import file_system;
import utils;
import task;
import serialize;

using namespace ffc;
using namespace ffc::core;
using namespace ffc::infra;
using namespace ffc::ai::model_manager;


int main(int argc, char** argv) {
    file_system::set_local_to_utf8();

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // test_task();
    // test_config();

    // from_json(config_json, core_options);

    // const std::string tmpPath = FileSystem::get_temp_path() + "/" + metadata::name;
    // FileSystem::remove_dir(tmpPath);
    //
    // ini_config ini_config;
    // ini_config.loadConfig();
    //
    // auto coreOptions = ini_config.getCoreOptions();
    // auto coreTask = ini_config.getCoreRunOptions();
    //
    // const auto core = std::make_shared<ffc::Core>(coreOptions);
    // const bool ok = core->Run(coreTask);
    //
    // if (!ok) {
    //     Logger::getInstance()->error("FaceFusionCpp failed to run. Maybe some of the tasks failed.");
    // }
    // FileSystem::remove_dir(tmpPath);
    return 0;
}
