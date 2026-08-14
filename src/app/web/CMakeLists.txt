# Web UI server library
add_library(app_web)
target_compile_features(app_web PUBLIC cxx_std_20)
target_sources(app_web
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        web_server.ixx
    PRIVATE
        web_server.cpp
)

find_package(Drogon CONFIG REQUIRED)

target_link_libraries(app_web
    PRIVATE
        app_version
        Drogon::Drogon
)
