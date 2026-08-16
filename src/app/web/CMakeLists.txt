# Web UI server library
add_library(app_web)
target_compile_features(app_web PUBLIC cxx_std_20)
target_sources(app_web
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        web_server.ixx
        task_types.ixx
        task_manager.ixx
    PRIVATE
        web_server.cpp
        task_manager.cpp
)

find_package(Drogon CONFIG REQUIRED)

target_link_libraries(app_web
    PRIVATE
        app_version
        config_core
        services_pipeline
        foundation_infrastructure
        Drogon::Drogon
)
