add_library(app_cli)
target_sources(app_cli
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        app_cli.ixx
        system_check.ixx
    PRIVATE
        app_cli.cpp
        system_check.cpp
)

find_package(indicators CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(app_cli
    PRIVATE
        config_core
        config_parser
        services_pipeline
        foundation_infrastructure
        foundation_media
        foundation_ai
        indicators::indicators
        CLI11::CLI11
        nlohmann_json::nlohmann_json
)

if(CUDA_ENABLED)
    target_compile_definitions(app_cli PRIVATE CUDA_ENABLED)
    target_link_libraries(app_cli PRIVATE CUDA::cudart)
endif()
