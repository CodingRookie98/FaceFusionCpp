target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_hub.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/yolo.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/scrfd.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/retina.ixx
)

target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_hub.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_detector_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/yolo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/scrfd.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/retina.cpp
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
