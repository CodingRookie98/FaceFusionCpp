target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_classifier_hub.ixx
        face_classifier_base.ixx
        fair_face.ixx
)

target_sources(${ProgramName} PRIVATE
        face_classifier_hub.cpp
        face_classifier_base.cpp
        fair_face.cpp
)
target_include_directories(${ProgramName} PRIVATE
        .
)
