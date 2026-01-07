target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/downloader.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/file_system.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/progress_bar.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/progress_observer.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_core.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_model.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_options.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_task.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.ixx
)

target_sources(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/boost_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/downloader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/file_system.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/progress_bar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
)

target_include_directories(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)