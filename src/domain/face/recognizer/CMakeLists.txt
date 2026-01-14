add_modules_library(domain_face_recognizer)

target_sources(domain_face_recognizer
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            face_recognizer.ixx
            face_recognizer_api.ixx
            face_recognizer_factory.ixx
            impl/arcface.ixx
    PRIVATE
        face_recognizer_factory.cpp
        impl/arcface.cpp
)

target_link_libraries(domain_face_recognizer
    PUBLIC
        foundation_ai
        domain_face
        ${OpenCV_LIBS}
)
