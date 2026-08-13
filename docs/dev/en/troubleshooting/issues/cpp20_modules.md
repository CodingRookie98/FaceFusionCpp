# Issue: C++20 Modules Not Supported (Compiler)

> **Document Control**
> - **Document ID**: FFC-DEV-EN-TS-ISSUE-CPP20-2026
> - **Version**: V1.0.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-13

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | Initialized document control info per documentation governance. |


## Description
Compilation fails with errors indicating that module semantics are not recognized or `ixx` files cannot be processed.

## Root Cause Analysis
- **Outdated Compiler**: C++20 Modules is a relatively new feature. MSVC requires 17.10+ and GCC requires 13+.
- **Missing CMake Support**: Older versions of CMake lacks the necessary module scanning capabilities for specific compilers.

## Solution
- **Upgrade Toolchain**: Ensure you are using the latest compilers as specified in the [Setup & Build Guide](../../guides/setup.md).
- **Enable Experimental Support**: When using GCC on Linux, ensure the correct CMake flags for module handling are enabled.

## Related Links
- [Setup & Build Guide](../../guides/setup.md)
