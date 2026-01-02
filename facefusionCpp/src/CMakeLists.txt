
target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/ai/inference_session.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/ai/model_manager.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/core.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/core_options.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/metadata.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/core/types.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/crypto.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/downloader.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/file_system.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/logger.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/progress_bar.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/progress_observer.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/serialize.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/thread_pool.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/utils.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/media/ffmpeg_runner.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/media/vision.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/task/task.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/task/core_task.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/task/task_manager.ixx
)

target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ai/inference_session.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ai/model_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/common_macros.h
        ${CMAKE_CURRENT_SOURCE_DIR}/core/core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/boost_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/crypto.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/downloader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/file_system.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/progress_bar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure/utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/media/ffmpeg_runner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/media/vision.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/task/core_task.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/task/task_manager.cpp
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(processors)
add_subdirectory(face)