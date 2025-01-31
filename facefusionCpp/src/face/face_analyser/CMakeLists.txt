target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_analyser.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_selector.ixx
)

target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_analyser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_selector.cpp
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(face_detector)
add_subdirectory(face_landmarker)
add_subdirectory(face_recognizer)
add_subdirectory(face_masker)
add_subdirectory(face_classifier)