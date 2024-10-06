target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_analyser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_analyser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifiers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifiers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detectors.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detectors.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarkers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_landmarkers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_maskers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_maskers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizers.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_selector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_selector.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(face_detector)
add_subdirectory(face_landmarker)
add_subdirectory(face_recognizer)
add_subdirectory(face_masker)
add_subdirectory(face_classifier)