target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_yolo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_yolo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_scrfd.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_scrfd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_retina.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_retina.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
