add_library(app_cli)
target_sources(app_cli
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        app_cli.ixx
    PRIVATE
        app_cli.cpp
)

find_package(indicators CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

target_link_libraries(app_cli
    PRIVATE
        config_core
        config_parser
        services_pipeline
        foundation_infrastructure
        indicators::indicators
        CLI11::CLI11
)
