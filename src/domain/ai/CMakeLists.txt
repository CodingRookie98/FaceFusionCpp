add_library(domain_ai)

find_package(nlohmann_json REQUIRED)

target_compile_features(domain_ai PUBLIC cxx_std_20)

set_target_properties(domain_ai PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF
)

target_sources(domain_ai
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            model_manager.ixx
    PRIVATE
        model_manager.cpp
)

target_link_libraries(domain_ai
    PUBLIC
        foundation_infrastructure
        nlohmann_json::nlohmann_json
)

if (MSVC)
    target_compile_options(domain_ai
        PRIVATE
            /permissive-
            /bigobj
            /wd5050
    )
endif()
