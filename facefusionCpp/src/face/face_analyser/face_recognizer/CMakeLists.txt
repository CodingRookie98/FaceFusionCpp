target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizer_hub.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizer_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/arc_w600k_r50.ixx
)
target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizer_hub.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/arc_w600k_r50.cpp
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
