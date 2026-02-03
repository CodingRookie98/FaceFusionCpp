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
        app_version
        indicators::indicators
        CLI11::CLI11
        nlohmann_json::nlohmann_json
)

find_package(CUDAToolkit)
if(CUDAToolkit_FOUND)
    target_link_libraries(app_cli PRIVATE CUDA::cudart)

    # Check for CUDA Driver
    target_compile_definitions(app_cli PRIVATE CUDA_ENABLED)

    # Try to find cuDNN
    find_path(CUDNN_INCLUDE_DIR cudnn.h PATHS ${CUDAToolkit_INCLUDE_DIRS} "$ENV{CUDNN_PATH}/include")
    find_library(CUDNN_LIBRARY cudnn PATHS ${CUDAToolkit_LIBRARY_DIR} "$ENV{CUDNN_PATH}/lib/x64")

    if(CUDNN_INCLUDE_DIR AND CUDNN_LIBRARY)
        target_compile_definitions(app_cli PRIVATE CUDNN_ENABLED)
        target_include_directories(app_cli PRIVATE ${CUDNN_INCLUDE_DIR})
        target_link_libraries(app_cli PRIVATE ${CUDNN_LIBRARY})
    endif()
endif()

# Try to find TensorRT
find_path(TENSORRT_INCLUDE_DIR NvInferVersion.h
    PATHS
        "$ENV{TensorRT_ROOT}/include"
)

find_library(TENSORRT_LIBRARY
    NAMES nvinfer nvinfer_10 nvinfer_9 nvinfer_8
    PATHS
        "$ENV{TensorRT_ROOT}/lib"
)

if(TENSORRT_INCLUDE_DIR AND TENSORRT_LIBRARY)
    message(STATUS "Found TensorRT Include: ${TENSORRT_INCLUDE_DIR}")
    message(STATUS "Found TensorRT Library: ${TENSORRT_LIBRARY}")
    target_compile_definitions(app_cli PRIVATE TENSORRT_ENABLED)
    target_include_directories(app_cli PRIVATE ${TENSORRT_INCLUDE_DIR})
    target_link_libraries(app_cli PRIVATE ${TENSORRT_LIBRARY})
else()
    message(STATUS "TensorRT NOT found for system check (Include: ${TENSORRT_INCLUDE_DIR}, Lib: ${TENSORRT_LIBRARY})")
endif()
