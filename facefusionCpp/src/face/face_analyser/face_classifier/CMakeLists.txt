target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_hub.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/fair_face.ixx
)

target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_hub.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fair_face.cpp
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
