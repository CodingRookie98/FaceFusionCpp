target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/region.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/occlusion.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_hub.ixx
)
target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/region.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/occlusion.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_hub.cpp
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
