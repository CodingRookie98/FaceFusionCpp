# Issue: Invalid Frame Dimensions (VideoWriter)

## Description
The pipeline fails during processing with the error: `VideoWriter: Invalid frame dimensions`, leading to output failure.

## Root Cause Analysis
- **Premature Initialization**: `PipelineRunner.cpp` attempts to open the `VideoWriter` using the original input video dimensions before the first AI-enhanced frame (e.g., 4x upsampled) arrives.
- **Dimension Mismatch**: When the first 4K enhanced frame reaches a Writer already locked to 720p, the underlying FFmpeg adapter throws an error.

## Solution
- **Deferred Open**: Modify the `PipelineRunner` logic to dynamically open the Writer only after the first valid result frame is produced, using that frame's `width`/`height`.

## Related Links
- [Pipeline Internals](../pipeline_internals.md) (To be updated)
