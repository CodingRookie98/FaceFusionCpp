module;
#include <string>
#include <vector>
#include <iostream>

export module app.cli;

import config.parser;
import services.pipeline.runner;
import foundation.infrastructure.logger;

export namespace app::cli {

class App {
public:
    static int run(int argc, char** argv);

private:
    static void print_version();
    static void run_pipeline(const std::string& config_path);
};

} // namespace app::cli
