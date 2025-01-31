target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_hub.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/t68By5.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/t2dfan.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/peppawutz.ixx
)
target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_hub.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/t68By5.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/t2dfan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarker_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/peppawutz.cpp
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
