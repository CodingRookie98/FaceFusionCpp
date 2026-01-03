target_sources(${target_app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_detector_hub.ixx
        face_detector_base.ixx
        yolo.ixx
        scrfd.ixx
        retina.ixx
)

target_sources(${target_app_facefusioncpp} PRIVATE
        face_detector_hub.cpp
        face_detector_base.cpp
        yolo.cpp
        scrfd.cpp
        retina.cpp
)
target_include_directories(${target_app_facefusioncpp} PRIVATE
        .
)
