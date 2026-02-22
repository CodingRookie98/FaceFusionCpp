# Issue: C++20 Modules Not Supported (Compiler)

## Description
Compilation fails with errors indicating that module semantics are not recognized or `ixx` files cannot be processed.

## Root Cause Analysis
- **Outdated Compiler**: C++20 Modules is a relatively new feature. MSVC requires 17.10+ and GCC requires 13+.
- **Missing CMake Support**: Older versions of CMake lacks the necessary module scanning capabilities for specific compilers.

## Solution
- **Upgrade Toolchain**: Ensure you are using the latest compilers as specified in the [Setup & Build Guide](../guides/setup.md).
- **Enable Experimental Support**: When using GCC on Linux, ensure the correct CMake flags for module handling are enabled.

## Related Links
- [Setup & Build Guide](../guides/setup.md)
