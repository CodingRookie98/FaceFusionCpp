function(setup_resource_copying TARGET_NAME)
    # 默认资源目录位于项目根目录的 assets
    set(ASSETS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/assets")

    # 输出目录依赖于全局设置的 CMAKE_RUNTIME_OUTPUT_DIRECTORY
    # 如果未设置，回退到 target 的输出目录 (这里简化处理，假设全局已设置)
    if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        message(FATAL_ERROR "CMAKE_RUNTIME_OUTPUT_DIRECTORY must be defined before calling setup_resource_copying")
    endif()

    # 我们希望将 assets 文件夹复制到 bin 目录下
    set(ASSETS_OUTPUT_ROOT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    # 1. 仅当目标不存在时创建目标
    if(NOT TARGET copy_resource_files)
        # 创建一个始终运行的自定义目标 (没有 OUTPUT，也没有 DEPENDS 具体文件)
        # 它会调用 cmake -P copy_assets.cmake
        add_custom_target(copy_resource_files
            COMMAND ${CMAKE_COMMAND}
                "-DASSETS_SOURCE_DIR=${ASSETS_SOURCE_DIR}"
                "-DASSETS_OUTPUT_ROOT_DIR=${ASSETS_OUTPUT_ROOT_DIR}"
                -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/copy_assets.cmake"
            COMMENT "Checking and updating resource files..."
            VERBATIM
        )
    endif()

    # 2. 绑定到目标
    add_dependencies(${TARGET_NAME} copy_resource_files)
endfunction()
