# Include config subdirectory
add_subdirectory(config)

# Define version library
add_library(app_version)
target_sources(app_version PUBLIC
        FILE_SET CXX_MODULES FILES
        version.ixx
        PRIVATE
        ${PROJECT_BINARY_DIR}/generated/version.cpp
)
add_dependencies(app_version update_version_info)
target_link_libraries(app_version PRIVATE foundation_infrastructure)

add_subdirectory(cli)

target_sources(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_fusion.cpp
)

target_link_libraries(${app_facefusioncpp} PRIVATE
        foundation_infrastructure
        config_core
        app_cli
        app_version
)
