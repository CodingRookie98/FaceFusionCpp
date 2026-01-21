add_modules_library(domain_face_detector)

target_sources(domain_face_detector
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            face_detector.ixx
            face_detector_api.ixx
            face_detector_types.ixx
            face_detector_factory.ixx
            internal_creators.ixx
            impl/face_detector_impl_base.ixx
    PRIVATE
        face_detector_factory.cpp
        impl/face_detector_impl_base.cpp
        impl/yolo.cpp
        impl/scrfd.cpp
        impl/retina.cpp
)

target_link_libraries(domain_face_detector
    PUBLIC
        foundation_infrastructure
        foundation_ai
        foundation_media
        domain_face
        ${OpenCV_LIBS}
)
