add_modules_library(domain_face_analyser)

target_sources(domain_face_analyser
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_analyser.ixx
            face_model_registry.ixx
    PRIVATE
        face_analyser.cpp
        face_model_registry.cpp
)

target_link_libraries(domain_face_analyser
    PUBLIC
        domain_face
        domain_face_detector
        domain_face_recognizer
        domain_face_classifier
        foundation_ai
        ${OpenCV_LIBS}
)
