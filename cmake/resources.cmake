function(setup_resource_copying TARGET_NAME)
    # 默认资源目录位于项目根目录的 assets
    set(ASSETS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/assets")
    
    # 输出目录依赖于全局设置的 CMAKE_RUNTIME_OUTPUT_DIRECTORY
    # 如果未设置，回退到 target 的输出目录 (这里简化处理，假设全局已设置)
    if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        message(FATAL_ERROR "CMAKE_RUNTIME_OUTPUT_DIRECTORY must be defined before calling setup_resource_copying")
    endif()
    
    set(ASSETS_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")
    set(ASSETS_TIMESTAMP "${CMAKE_BINARY_DIR}/assets_copy.timestamp")

    # 1. 扫描所有资源文件 (使用 CONFIGURE_DEPENDS 确保新增文件时 CMake 能感知)
    file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${ASSETS_SOURCE_DIR}/*")

    # 2. 仅当目标不存在时创建命令和目标 (避免重复定义错误)
    if(NOT TARGET copy_resource_files)
        # 创建自定义命令：仅当 ASSET_FILES 变化时运行
        add_custom_command(
            OUTPUT ${ASSETS_TIMESTAMP}
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${ASSETS_OUTPUT_DIR}"
            COMMAND ${CMAKE_COMMAND} -E touch ${ASSETS_TIMESTAMP}
            DEPENDS ${ASSET_FILES}
            COMMENT "Updating resource files in ${ASSETS_OUTPUT_DIR}..."
            VERBATIM
        )

        add_custom_target(copy_resource_files DEPENDS ${ASSETS_TIMESTAMP})
    endif()

    # 3. 绑定到目标
    add_dependencies(${TARGET_NAME} copy_resource_files)
endfunction()
