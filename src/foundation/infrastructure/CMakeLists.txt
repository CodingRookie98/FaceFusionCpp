# Find Dependencies
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
message(STATUS "Checking for CUDA Toolkit...")
find_package(CUDAToolkit)

if(CUDAToolkit_FOUND)
    set(CUDA_ENABLED ON)
    message(STATUS "CUDA Toolkit found at ${CUDAToolkit_LIBRARY_DIR}, enabling GPU metrics sampling")
else()
    message(STATUS "CUDA Toolkit NOT found, GPU metrics sampling will be disabled")
endif()
# indicators is usually header-only, assuming it's available or we might need to find/add it.
# For now assuming it is available via include path or fetchcontent in root.

add_modules_library(foundation_infrastructure STATIC)

target_sources(foundation_infrastructure
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            core_utils.ixx
            core_utils_random.ixx
            core_utils_conversion.ixx
            core_utils_encoding.ixx
            network.ixx
            file_system.ixx
            concurrent_file_system.ixx
            crypto.ixx
            concurrent_crypto.ixx
            concurrent_queue.ixx
            logger_types.ixx
            logger.ixx
            progress.ixx
            scoped_timer.ixx
            process.ixx
            thread_pool.ixx
            cuda_utils.ixx
            console.ixx
    PRIVATE
        core_utils.cpp
        network.cpp
        file_system.cpp
        concurrent_file_system.cpp
        crypto.cpp
        concurrent_crypto.cpp
        logger.cpp
        progress.cpp
        scoped_timer.cpp
        process.cpp
        thread_pool.cpp
        cuda_utils.cpp
        console.cpp
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

if(CUDA_ENABLED)
    target_compile_definitions(foundation_infrastructure PRIVATE CUDA_ENABLED)
    target_link_libraries(foundation_infrastructure PRIVATE CUDA::cudart)
endif()
