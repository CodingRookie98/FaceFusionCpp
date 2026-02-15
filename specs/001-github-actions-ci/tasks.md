# Tasks: GitHub Actions CI/CD Pipeline

**Feature**: GitHub Actions CI/CD Pipeline
**Status**: Pending
**Branch**: `001-github-actions-ci`

## Phase 1: Setup
**Goal**: Initialize CI configuration structure and basic environment scripts.

- [ ] T001 Create .github/workflows directory structure
- [ ] T002 Create initial CI workflow file in .github/workflows/ci.yml
- [ ] T003 Create release workflow file in .github/workflows/release.yml

## Phase 2: Foundational (Blocking)
**Goal**: Establish reusable actions and scripts for environment setup across workflows.
**Independent Test**: Scripts run locally and actions pass linting.

- [ ] T004 Create composite action for vcpkg setup and caching in .github/actions/setup-vcpkg/action.yml
- [ ] T005 Create composite action for CUDA toolkit installation (Linux) in .github/actions/setup-cuda-linux/action.yml
- [ ] T006 Create composite action for CUDA toolkit installation (Windows) in .github/actions/setup-cuda-windows/action.yml
- [ ] T007 Update build.py to support CI-specific flags (if needed) or verify existing flags

## Phase 3: User Story 1 - Automated Build & Test (P1)
**Goal**: Implement the main CI pipeline for Pull Requests and Pushes.
**Story**: [US1] Automated Build & Test on Push
**Independent Test**: Pushing a commit triggers the workflow, installs dependencies, builds, and runs unit tests.

- [ ] T008 [US1] Implement checkout and environment setup steps in .github/workflows/ci.yml
- [ ] T009 [P] [US1] Add Linux (Ubuntu) build job configuration to ci.yml
- [ ] T010 [P] [US1] Add Windows build job configuration to ci.yml
- [ ] T011 [US1] Add unit test execution step to ci.yml (Linux & Windows)
- [ ] T012 [US1] Add code formatting check step to ci.yml
- [ ] T013 [US1] Implement workflow cancellation for concurrent pushes in ci.yml

## Phase 4: User Story 2 - Release Artifact Generation (P2)
**Goal**: Automate release packaging and publishing on tag push.
**Story**: [US2] Release Artifact Generation
**Independent Test**: Pushing a v* tag triggers release workflow, builds release binary, creates zip, and publishes GitHub Release.

- [ ] T014 [US2] Implement checkout and release build steps in .github/workflows/release.yml
- [ ] T015 [US2] Add packaging step (zip creation excluding models) to release.yml
- [ ] T016 [US2] Add GitHub Release creation and asset upload step to release.yml

## Phase 5: Polish
**Goal**: Finalize configurations and documentation.

- [ ] T017 Add build status badge to README.md
- [ ] T018 Document CI/CD process and manual model download instructions in docs/dev/ci-cd.md

## Implementation Strategy
- **MVP**: Focus on T001-T012 to get the build and test pipeline green.
- **Incremental**: Add Release workflow (T014-T016) after CI is stable.
- **Parallelism**: Linux and Windows job configurations (T009, T010) can be implemented in parallel.

## Dependencies
1. Phase 1 & 2 must be completed before Phase 3.
2. Phase 3 (CI) should be stable before Phase 4 (Release) is fully relied upon, though they can be implemented sequentially.
