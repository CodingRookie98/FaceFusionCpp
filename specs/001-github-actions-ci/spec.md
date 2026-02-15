# Feature Specification: GitHub Actions CI/CD Pipeline

**Feature Branch**: `001-github-actions-ci`
**Created**: 2026-02-15
**Status**: Draft
**Input**: User description: "我们开始进入的CI/CD阶段，使用github workflows 和 github actions"

## User Scenarios & Testing *(mandatory)*

## Clarifications
### Session 2026-02-15
- Q: How should the CI pipeline handle hardware acceleration dependencies? → A: **Compilation Check**: Install CUDA Toolkit on runners to verify compilation. Skips GPU execution. (Use caching like vcpkg to mitigate slowness).
- Q: How will we provide the Windows build environment? → A: **Standard Runner + Cache**: Install dependencies (CUDA, vcpkg) on standard runner, heavily relying on caching.
- Q: What version of compilers should we use? → A: **Strict Version Pinning**: Ubuntu uses system default (pinned via image version), Windows uses `latest` image (Visual Studio 2022).
- Q: Where should vcpkg binary artifacts be stored? → A: **GitHub Packages (Nuget)**: Publish binaries as packages for better persistence and sharing across workflows.
- Q: Where do the model files come from during the release build? → A: **Runtime Download**: Models are NOT bundled. The application downloads them at runtime (or user manually).

### User Story 1 - Automated Build & Test on Push (Priority: P1)

As a developer, I want my code to be automatically built and tested whenever I push changes or open a pull request, so that I can catch regressions early.

**Why this priority**: Core CI functionality ensures code quality and prevents broken builds from merging.

**Independent Test**: Push a commit to a feature branch and verify that the GitHub Action workflow triggers, builds the project, and passes tests.

**Acceptance Scenarios**:

1. **Given** a new commit pushed to `master`, `main`, `dev`, or `*dev`, **When** the CI workflow runs, **Then** the project compiles successfully on both Linux (Ubuntu) and Windows.
2. **Given** a pull request targeting `master`, `main`, or `dev`, **When** the CI workflow runs, **Then** all unit tests pass (`python build.py --action test --test-label unit`).
3. **Given** code that violates formatting rules, **When** the lint check runs, **Then** the workflow fails.

---

### User Story 2 - Release Artifact Generation (Priority: P2)

As a maintainer, I want release binaries to be automatically generated and uploaded when I push a version tag, so that users can easily download the software.

**Why this priority**: Automates the distribution process, reducing manual errors and effort.

**Independent Test**: Push a tag like `v0.1.0` and verify a GitHub Release is drafted/published with assets.

**Acceptance Scenarios**:

1. **Given** a git tag starting with `v` is pushed (on `master` or `main`), **When** the CD workflow runs, **Then** it builds the project in Release mode.
2. **Given** a successful release build, **When** the workflow completes, **Then** a ZIP package containing the executable and assets is uploaded to the GitHub Release.

### Edge Cases

- **Build Timeouts**: Builds exceeding GitHub Actions limits (e.g., 60 mins) should be cancelled to save resources.
- **Concurrent Builds**: Multiple pushes to the same branch should cancel previous redundant builds to save runners.
- **Flaky Tests**: Tests that pass intermittently should be identified and either fixed or isolated to prevent blocking the pipeline.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST provide a GitHub Actions workflow (`ci.yml`) that triggers on `push` and `pull_request` events.
  - **Trigger Rule**: Push triggers only on `master`, `main`, `dev`, and branches matching `*dev`.
- **FR-002**: The CI workflow MUST execute the build script (`python build.py --action build`) on **Ubuntu** (latest) and **Windows** (latest) runners. It MUST install CUDA/TensorRT build dependencies to verify compilation.
- **FR-003**: The CI workflow MUST run unit tests via `python build.py --action test --test-label unit` and fail the job if tests fail.
- **FR-004**: The CI workflow MUST check code formatting using the project's format script (`python scripts/format_code.py` or similar check).
- **FR-005**: The system MUST implement aggressive dependency caching using **vcpkg binary caching with GitHub Packages (NuGet)** as the backend storage.
- **FR-006**: The system MUST provide a separate release workflow (`release.yml`) that triggers only on tags matching `v*`.
- **FR-007**: The release workflow MUST package the binary and required **configuration files only** into a downloadable archive (ZIP/Tarball). **Models are EXCLUDED** (downloaded at runtime).
- **FR-008**: The release workflow MUST publish the artifacts to GitHub Releases.

### Key Entities *(include if feature involves data)*

- **CI Workflow**: `.github/workflows/ci.yml`
- **Release Workflow**: `.github/workflows/release.yml`

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: CI pipeline completes execution (build + test) within 20 minutes for a standard commit.
- **SC-002**: 100% of pull requests are automatically verified for compilation and test passing before merge.
- **SC-003**: Release artifacts are generated automatically for every version tag without manual packaging.
- **SC-004**: Both Windows and Linux builds are verified consistently.
