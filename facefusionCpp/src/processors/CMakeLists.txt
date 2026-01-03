add_subdirectory(./face_swapper)
add_subdirectory(./face_enhancer)
add_subdirectory(./expression_restorer)
add_subdirectory(./frame_enhancer)
target_sources(${target_app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_hub.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_pool.ixx
)
target_sources(${target_app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_hub.cpp
)
target_include_directories(${target_app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
