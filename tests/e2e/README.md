# FaceFusionCpp End-to-End (E2E) Tests

This directory contains the End-to-End testing infrastructure for FaceFusionCpp. Unlike unit and integration tests which run via GTest/CMake, these tests execute the compiled `FaceFusionCpp` binary directly via the command line to verify the complete user workflow.

## 🎯 Purpose

*   **Verify User Workflows**: Ensure the CLI accepts arguments correctly, loads configurations, executes the pipeline, and produces valid output files.
*   **Release Validation**: Serve as the final gatekeeper before a release.
*   **Hardware Compatibility**: Verify performance and stability on different GPUs.

## 📂 Directory Structure

```
tests/e2e/
├── configs/            # YAML configuration files for different test scenarios
├── expected/           # Expected output references (e.g., SSIM baselines)
├── scripts/            # Python automation scripts
│   ├── run_e2e.py      # Main execution script
│   └── verify_output.py # Output validation utility
└── README.md           # This document
```

## 📤 Output Location

All E2E test outputs are generated in a structured directory within the binary folder:
`build/{preset}/bin/output/test/e2e/{config_name}/`

Example:
If running `e2e_image_single.yaml` in Debug mode:
`build/linux-x64-debug/bin/output/test/e2e/e2e_image_single/`

## 🚀 How to Run

### Prerequisites

1.  **Build Release Binary**:
    ```bash
    python build.py --config Release --action build
    ```
2.  **Install Dependencies**:
    *   Python 3.8+
    *   FFmpeg (for video processing and validation)
    *   `ffprobe` must be in your PATH.

### Execution Steps

1.  **Navigate to the Binary Directory**:
    > **Critical**: You MUST run the tests from the directory where the `FaceFusionCpp` executable is located to ensure relative paths in configs work correctly.

    ```bash
    # Example for Linux/WSL
    cd build/linux-x64-release/bin
    ```

2.  **Run the E2E Test Runner**:
    ```bash
    # Run all E2E tests
    python3 ../../../tests/e2e/scripts/run_e2e.py

    # Run only image tests
    python3 ../../../tests/e2e/scripts/run_e2e.py --filter image

    # Run only video tests
    python3 ../../../tests/e2e/scripts/run_e2e.py --filter video
    ```

## 🧩 Test Scenarios

### Image Tests
*   **Baseline (512x512)**: Basic face swap functionality.
*   **720p / 2K**: Performance and memory usage under higher resolutions.
*   **Palette/Edge Cases**: Handling of non-standard image formats.
*   **Multi-Processor**: Complex pipelines (Swap -> Enhance -> Frame Enhance).

### Video Tests
*   **Baseline**: Basic video face swap.
*   **Checkpoint**: Resume functionality (`enable_resume: true`).
*   **Graceful Shutdown**: Handling `SIGINT` (Ctrl+C).

### System Tests
*   **System Check**: `FaceFusionCpp --system-check`
*   **Config Validation**: Error reporting for invalid configs.
*   **Resource Monitoring**: Memory leaks and VRAM usage.

## ⚠️ Troubleshooting

*   **"File not found"**: Ensure you are running the script from the **bin** directory.
*   **"Model load failed"**: Check if the model files are present in the `models/` directory relative to the binary.
