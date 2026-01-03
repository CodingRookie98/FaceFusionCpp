target_sources(${target_app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/in_swapper.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper.ixx
)

target_sources(${target_app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/in_swapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_base.cpp
)

target_include_directories(${target_app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
