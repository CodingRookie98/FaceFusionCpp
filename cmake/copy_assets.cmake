# Script to copy assets at build time
# Expected Variables:
# - ASSETS_SOURCE_DIR: Path to the source assets directory
# - ASSETS_OUTPUT_ROOT_DIR: Path to the destination root directory (e.g., bin)

message(STATUS "CopyAssets: Source=[${ASSETS_SOURCE_DIR}] Dest=[${ASSETS_OUTPUT_ROOT_DIR}]")

if(NOT EXISTS "${ASSETS_SOURCE_DIR}")
    message(FATAL_ERROR "ASSETS_SOURCE_DIR does not exist: ${ASSETS_SOURCE_DIR}")
endif()

if(NOT EXISTS "${ASSETS_OUTPUT_ROOT_DIR}")
    # Ideally should not happen if build system is correct, but we can try to make it
    file(MAKE_DIRECTORY "${ASSETS_OUTPUT_ROOT_DIR}")
endif()

# file(COPY ...) copies the source directory INTO the destination directory.
# So copying '.../assets' into '.../bin' results in '.../bin/assets'
file(COPY "${ASSETS_SOURCE_DIR}" DESTINATION "${ASSETS_OUTPUT_ROOT_DIR}")
