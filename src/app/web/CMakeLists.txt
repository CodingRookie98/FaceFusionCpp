# Web UI server library
add_library(app_web)
target_compile_features(app_web PUBLIC cxx_std_20)
target_sources(app_web
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        web_server.ixx
        task_types.ixx
        task_manager.ixx
        pipeline_executor.ixx
    PRIVATE
        web_server.cpp
        task_manager.cpp
        pipeline_executor.cpp
)

find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(app_web
    PRIVATE
        app_version
        config_core
        config_parser
        services_pipeline
        foundation_infrastructure
        nlohmann_json::nlohmann_json
        Drogon::Drogon
)
