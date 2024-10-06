target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_region.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_region.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_occlusion.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_masker_occlusion.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
