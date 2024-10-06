target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_fair_face.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_classifier_fair_face.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
