
target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/core/core.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/core_options.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/metadata.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/types.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/media/ffmpeg_runner.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/media/vision.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/task/task.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/task/core_task.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/task/task_manager.ixx
)

target_sources(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core/core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/media/ffmpeg_runner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/media/vision.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/task/core_task.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/task/task_manager.cpp
)

target_include_directories(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(processors)
add_subdirectory(face)
add_subdirectory(infrastructure)