
target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/vision.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/inference_session.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/downloader.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/file_system.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/ini_config.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/progress_bar.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_runner.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/model_manager.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core_task.ixx
)

target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vision.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/inference_session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/downloader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/file_system.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ini_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/progress_bar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_runner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/common_macros.h
        ${CMAKE_CURRENT_SOURCE_DIR}/core_task.cpp
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(processors)
add_subdirectory(face)