# Issue: Video Test Timeout (Strict Memory)

> **Document Control**
> - **Document ID**: FFC-DEV-EN-TS-ISSUE-VTO-2026
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
During integration testing, `PipelineRunnerVideoTest.ProcessVideoStrictMemory` failed to complete within 120 seconds, triggering a timeout.

## Root Cause Analysis
1. **Build Mode**: The C++ Debug build is extremely slow due to a lack of optimizations and extra safety checks.
2. **Memory Strategy**: The `strict` mode involves frequent disk I/O to temporary files to reduce RAM usage, which significantly increases overhead.
3. **Video Specs**: The test video `slideshow_scaled.mp4` with RealESRGAN x4 upsampling takes a long time to encode, even for short clips.

## Solution
- **Short-term**: Increase the timeout for this specific test case in `tests/integration/CMakeLists.txt` (e.g., to 600s).
- **Long-term**: Switch to lighter weight models (e.g., `real_esrgan_x2_fp16`) or reduce the number of frames in the test video for integration tests.

## Related Links
- [System Configuration Guide](../../../../user/en/configuration_guide.md#memory-strategy)
