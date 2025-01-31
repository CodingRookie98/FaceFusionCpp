target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/inSwapper.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper.ixx
)

target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inSwapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_base.cpp
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
