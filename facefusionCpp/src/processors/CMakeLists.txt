add_subdirectory(./face_swapper)
add_subdirectory(./face_enhancer)
add_subdirectory(./expression_restorer)
add_subdirectory(./frame_enhancer)
target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/processor_base.h
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
