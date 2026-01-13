add_modules_library(domain_face_classifier)

target_sources(domain_face_classifier
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_classifier.ixx
            face_classifier_api.ixx
            face_classifier_factory.ixx
            impl/fair_face.ixx
    PRIVATE
        face_classifier_factory.cpp
        impl/fair_face.cpp
)

target_link_libraries(domain_face_classifier
    PUBLIC
        domain_face
        foundation_ai
        ${OpenCV_LIBS}
)
