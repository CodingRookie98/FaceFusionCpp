target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_recognizer_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fr_arc_w_600_k_r_50.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fr_arc_w_600_k_r_50.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
