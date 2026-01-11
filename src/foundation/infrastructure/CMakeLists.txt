cmake_minimum_required(VERSION 3.28)
project(foundation_infrastructure LANGUAGES CXX)

# Find Dependencies
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
# indicators is usually header-only, assuming it's available or we might need to find/add it.
# For now assuming it is available via include path or fetchcontent in root.

add_library(foundation_infrastructure STATIC)

target_compile_features(foundation_infrastructure PUBLIC cxx_std_20)

set_target_properties(foundation_infrastructure PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF
)

target_sources(foundation_infrastructure
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            core_utils.ixx
            core_utils_random.ixx
            core_utils_conversion.ixx
            network.ixx
            file_system.ixx
            concurrent_file_system.ixx
            crypto.ixx
            concurrent_crypto.ixx
            logger.ixx
            progress.ixx
            thread_pool.ixx
            test_support.ixx
    PRIVATE
        core_utils.cpp
        network.cpp
        file_system.cpp
        concurrent_file_system.cpp
        crypto.cpp
        concurrent_crypto.cpp
        logger.cpp
        progress.cpp
        thread_pool.cpp
        test_support.cpp
)

target_link_libraries(foundation_infrastructure
    PUBLIC
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
        CURL::libcurl
        spdlog::spdlog
        OpenSSL::SSL
        OpenSSL::Crypto
        # indicators::indicators # If it has a target
)

if (MSVC)
    target_compile_options(foundation_infrastructure
        PRIVATE
            /permissive-
            /bigobj
            /wd5050 # Header unit warning
    )
endif()
