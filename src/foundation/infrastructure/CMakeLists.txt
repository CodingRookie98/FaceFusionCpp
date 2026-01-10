cmake_minimum_required(VERSION 3.30)
project(infrastructure LANGUAGES CXX)

find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

add_library(infrastructure_lib STATIC)

target_compile_features(infrastructure_lib
        PUBLIC cxx_std_20
)
set_target_properties(infrastructure_lib PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
)
target_sources(infrastructure_lib
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        core_utils.ixx
        network.ixx
        PRIVATE
        core_utils.cpp
        network.cpp
)

target_link_libraries(infrastructure_lib
        PUBLIC
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

if (MSVC)
    target_compile_options(infrastructure_lib
            PRIVATE
            /permissive-
            /bigobj
    )
endif ()
