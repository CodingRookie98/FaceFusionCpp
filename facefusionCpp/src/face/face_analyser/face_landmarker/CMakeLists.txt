target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_68By5.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_68By5.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_2dfan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_2dfan.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_peppawutz.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_peppawutz.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
