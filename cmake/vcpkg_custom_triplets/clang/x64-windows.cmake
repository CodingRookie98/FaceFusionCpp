set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PLATFORM_TOOLSET "C:/devTools/msys64/clang64")

set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -stdlib=libc++")
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} ")
message(STATUS "Custom triplet for clang: VCPKG_CXX_FLAGS -> ${VCPKG_CXX_FLAGS}")
message(STATUS "Custom triplet for clang: VCPKG_C_FLAGS -> ${VCPKG_C_FLAGS}")
