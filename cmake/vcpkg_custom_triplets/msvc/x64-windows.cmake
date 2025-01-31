set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} /utf-8")
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} /utf-8")
message(STATUS "Custom triplet for msvc: VCPKG_CXX_FLAGS -> ${VCPKG_CXX_FLAGS}")
message(STATUS "Custom triplet for msvc: VCPKG_C_FLAGS -> ${VCPKG_C_FLAGS}")
