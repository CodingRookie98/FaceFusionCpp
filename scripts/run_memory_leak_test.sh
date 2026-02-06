#!/bin/bash
# scripts/run_memory_leak_test.sh

# Build with ASan enabled
python build.py --preset debug --cmake-args "-DENABLE_ASAN=ON"

# Determine the build directory (handling preset logic)
# build.py typically creates build/linux-x64-debug or similar.
# We'll try to find the binary.

BUILD_DIR="build/linux-x64-debug/bin"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory $BUILD_DIR not found."
    exit 1
fi

cd "$BUILD_DIR"

# Run integration tests with filter for E2E or relevant tests
# The plan suggests filtering *E2E*, but we also added CheckpointResume and EdgeCases.
# Let's run those too.
./integration_tests --gtest_filter="*E2E*:*CheckpointResume*:*EdgeCases*"

# ASan will report leaks to stderr and exit with non-zero if configured to do so.
# Default ASan behavior usually aborts on error.
