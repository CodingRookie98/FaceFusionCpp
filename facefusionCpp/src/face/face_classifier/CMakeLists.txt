target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_classifier_hub.ixx
        face_classifier_base.ixx
        fair_face.ixx
)

target_sources(${app_facefusioncpp} PRIVATE
        face_classifier_hub.cpp
        face_classifier_base.cpp
        fair_face.cpp
)
target_include_directories(${app_facefusioncpp} PRIVATE
        .
)
