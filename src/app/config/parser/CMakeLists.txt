# Config Parser Library
# YAML 解析与配置校验

add_modules_library(config_parser)

target_sources(config_parser
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            config_parser.ixx
)

target_sources(config_parser
    PRIVATE
        config_parser.cpp
)

target_link_libraries(config_parser
    PUBLIC
        config_core
    PRIVATE
        foundation_infrastructure  # yaml-cpp, json
)
