# Quickstart: CI/CD Pipeline

## Overview

This project uses GitHub Actions for Continuous Integration (CI) and Continuous Deployment (CD).

- **CI**: Runs on every push and PR to verify build, tests, and formatting.
- **CD**: Runs on version tags (`v*`) to create releases.

## Local Prerequisites

To replicate CI steps locally:

1.  **Format Check**:
    ```bash
    python scripts/format_code.py
    ```
2.  **Unit Tests**:
    ```bash
    python build.py --action test --test-label unit
    ```
3.  **Build (Debug)**:
    ```bash
    python build.py --action build --preset linux-x64-debug  # or windows-x64-debug
    ```

## Triggering a Release

1.  Ensure `master` branch is up to date and passing CI.
2.  Tag the commit with a semantic version (must start with `v`):
    ```bash
    git tag v1.0.0
    git push origin v1.0.0
    ```
3.  Monitor the "Release" workflow in the GitHub Actions tab.
4.  Once complete, verify the draft/published release on the Releases page.

## Troubleshooting CI

-   **Vcpkg Cache Miss**: If builds are slow, check if the NuGet feed credentials are valid and if `VCPKG_BINARY_SOURCES` is set correctly in the workflow.
-   **CUDA Errors**: Ensure the runner installed the correct CUDA version matching `CMakeLists.txt` requirements.
-   **Test Failures**: Inspect the workflow logs. Artifacts (logs) might be uploaded if configured.

## Secrets Required

-   `GITHUB_TOKEN`: Automatically provided by GitHub Actions (used for Packages and Releases).
-   No external secrets currently required.
