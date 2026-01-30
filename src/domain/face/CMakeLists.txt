find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(flatbuffers CONFIG REQUIRED)

add_modules_library(domain_face)

target_sources(domain_face
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_types.ixx
            face.ixx
            face_helper.ixx
            face_selector.ixx
            face_store.ixx
    PRIVATE
        face_impl.cpp
        face_helper.cpp
        face_selector.cpp
        face_store.cpp
)

target_include_directories(domain_face PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/schema)

target_link_libraries(domain_face
    PUBLIC
        ${OpenCV_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        domain_common
        foundation_ai
        domain_ai
        flatbuffers::flatbuffers
)


add_subdirectory(detector)
add_subdirectory(classifier)
add_subdirectory(landmarker)
add_subdirectory(recognizer)
add_subdirectory(masker)
add_subdirectory(analyser)
add_subdirectory(swapper)
add_subdirectory(enhancer)
add_subdirectory(expression)
