# Config Core Library
# 配置核心数据结构与类型定义

add_modules_library(config_core)

target_sources(config_core
    PRIVATE
        error_codes.cpp
        config_validator.cpp
        config_merger.cpp
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            config_types.ixx
            app_config.ixx
            task_config.ixx
            config_validator.ixx
            config_merger.ixx
            inference_options.ixx
)

# Inference provider resolution depends on the ONNX Runtime session contract
# (Foundation layer, legitimate App -> Foundation dependency).
target_link_libraries(config_core PUBLIC foundation_ai)

# Parser subdirectory
add_subdirectory(parser)
