# Phase 0: Research & Decisions

**Feature**: GitHub Actions CI/CD Pipeline
**Date**: 2026-02-15

## 1. Vcpkg Binary Caching Strategy

**Decision**: Use **GitHub Packages (NuGet)** as the vcpkg binary caching backend.
**Rationale**: 
- Provides persistent storage across workflow runs and branches.
- Native integration with GitHub Actions authentication (`GITHUB_TOKEN`).
- More reliable than local `@actions/cache` for large binary sets (C++ dependencies).
- Supports access control within the organization/user scope.
**Alternatives Considered**:
- **GitHub Actions Cache (`@actions/cache`)**: Free and easy, but limited to 10GB total and has aggressive eviction policies. Good for short-term, bad for long-term stability of heavy C++ deps.
- **Azure Blob/AWS S3**: Infinite scale but requires external cloud credentials and configuration. Overkill for current scale.

## 2. CUDA/TensorRT in CI

**Decision**: Install CUDA Toolkit but **SKIP** GPU execution.
**Rationale**:
- GitHub hosted runners are CPU-only.
- We need to verify that code *compiles* against CUDA headers and links against stubs.
- Running tests that require actual GPU inference will fail or be extremely slow (emulation).
- Unit tests should mock GPU interactions or rely on CPU fallbacks where possible.
**Implementation Detail**:
- Linux: Use `networkinstaller` or apt to install minimal CUDA toolkit.
- Windows: Use silent installer for CUDA toolkit.
- CMake: Configure with `-DFACEFUSION_ENABLE_GPU=ON` to force CUDA codepaths compilation.

## 3. Windows Runner Environment

**Decision**: Use `windows-latest` (VS2022) standard runners.
**Rationale**:
- Sufficient for building C++20 projects.
- Comes with Chocolatey for easy tool installation (if needed).
- Pre-installed with CMake, Ninja, Python.
- Cost-effective compared to self-hosted.

## 4. Release Artifact Packaging

**Decision**: Use existing `python build.py --action package` functionality.
**Rationale**:
- The project already has a working packaging mechanism via `build.py` (which wraps `cpack`).
- Avoids duplicating packaging logic in YAML files.
- Ensures consistency with local builds.
- **Constraint**: Must verify `CPackConfig.cmake` is correctly generated and respects the "no models" requirement.

## 5. Workflow Triggers (REVISED)

**Decision**:
- `ci.yml` (Build & Test):
  - `push`: branches `[ "master", "main", "dev", "*dev" ]`
  - `pull_request`: branches `[ "master", "main", "dev" ]`
- `release.yml` (Package & Publish):
  - `push`: tags `[ "v*" ]` (ONLY if created on `master` or `main` - enforce via branch protection or workflow logic if possible, otherwise rely on tag discipline).
  - Note: GitHub Actions tag triggers don't explicitly filter by "source branch" easily without hacks. Standard practice is to assume `v*` tags are pushed to stable branches.
