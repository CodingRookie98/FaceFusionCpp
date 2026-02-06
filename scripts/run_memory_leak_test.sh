#!/bin/bash
set -e

cd "$(dirname "$0")/.."

cmake --preset linux-debug -DENABLE_ASAN=ON

cmake --build --preset linux-debug --parallel $(nproc)

cd build/linux-x64-debug/bin
./app_test_pipeline --gtest_filter="*MemoryMonitoring*"

# ASan will report leaks to stderr and exit with non-zero if configured to do so.
# Default ASan behavior usually aborts on error.
