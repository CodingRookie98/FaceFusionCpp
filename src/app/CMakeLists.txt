# Include config subdirectory
add_subdirectory(config)

target_sources(${app_facefusioncpp} PUBLIC
        FILE_SET CXX_MODULES FILES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_fusion.cpp
)

target_link_libraries(${app_facefusioncpp} PRIVATE
        foundation_infrastructure
        config_core
)
