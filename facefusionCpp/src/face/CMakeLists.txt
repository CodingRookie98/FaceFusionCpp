target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_store.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_store.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_helper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_helper.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(face_analyser)
