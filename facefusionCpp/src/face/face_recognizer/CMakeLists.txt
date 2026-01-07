target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_recognizer_hub.ixx
        face_recognizer_base.ixx
        arc_w600k_r50.ixx
)
target_sources(${app_facefusioncpp} PRIVATE
        face_recognizer_hub.cpp
        face_recognizer_base.cpp
        arc_w600k_r50.cpp
)

target_include_directories(${app_facefusioncpp} PRIVATE
        .
)
