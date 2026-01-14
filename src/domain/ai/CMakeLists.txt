add_modules_library(domain_ai)

find_package(nlohmann_json REQUIRED)

target_sources(domain_ai
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            model_repository.ixx
    PRIVATE
        model_repository.cpp
)

target_link_libraries(domain_ai
    PUBLIC
        foundation_infrastructure
        nlohmann_json::nlohmann_json
)
