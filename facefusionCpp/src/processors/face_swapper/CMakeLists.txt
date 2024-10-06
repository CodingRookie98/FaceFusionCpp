target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_inswaper_128.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_inswaper_128.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_helper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_swapper_helper.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
