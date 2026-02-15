# FaceFusionCpp Constitution
<!-- Project Constitution: The Supreme Law of the Codebase -->

<!--
SYNC IMPACT REPORT
==================
Version: 1.0.0 -> 1.1.0
Change Type: Minor (Relaxed Testing Constraints)

Principles Modified:
- I. Test-Driven Development (TDD): Clarified scope (Unit Tests Mandatory, others optional).
- V. Workflow & Governance: Updated verification command to focus on unit tests.

Template Status:
- plan-template.md: ✅ Compatible
- spec-template.md: ✅ Compatible
- tasks-template.md: ⚠ Updated to reflect Unit Test mandate

Follow-up:
- None.
-->

## Core Principles

### I. Test-Driven Development (TDD)
<!-- NON-NEGOTIABLE -->
TDD is the mandatory development methodology for this project, specifically for **Unit Tests**. No production code may be written without a failing unit test.
1. **Red**: Write a failing unit test that defines the expected behavior.
2. **Green**: Write the minimum amount of code to make the test pass.
3. **Refactor**: Clean up the code while keeping tests green.
**Note**: Integration and E2E tests are **optional** and only required if explicitly requested by the user.

### II. Language Policy (Chinese First)
<!-- User Mandate -->
All conversations, reasoning, and documentation must be conducted in **Chinese** unless the user explicitly requests otherwise.
- **Thinking**: Agent reasoning must be in Chinese.
- **Communication**: Responses to the user must be in Chinese.
- **Documentation**: Project documentation (docs/, plans, specs) must be in Chinese.
- **Code**: Code comments and variable naming conventions follow standard English practices (Snake Case/Camel Case) unless specifically instructed otherwise, but explanatory block comments may be in Chinese if it aids clarity for the team.

### III. Modern C++ Standards
<!-- C++20 Strict -->
The project enforces **C++20** standards.
- **Modules**: Use `.ixx` for interfaces and `.cpp` for implementations. Avoid legacy headers where possible.
- **Memory Management**: Use Smart Pointers (std::unique_ptr, std::shared_ptr) and RAII. No manual `new`/`delete`.
- **Design**: Prefer Composition over Inheritance. Use PIMPL to hide implementation details.
- **Safety**: No `const_cast` or C-style casts.

### IV. Strict Layered Architecture
<!-- Architectural Integrity -->
The system follows a strict 5-Layered Architecture. Dependencies must flow unidirectionally (App -> Services -> Domain -> Platform -> Foundation).
- **App**: CLI, Configuration.
- **Services**: Business logic orchestration (PipelineRunner).
- **Domain**: Core business entities (Face, Image).
- **Platform**: OS abstractions (FileSystem).
- **Foundation**: Low-level utilities (AI Inference, Logging).
Circular dependencies are strictly forbidden.

### V. Workflow & Governance
<!-- Process Compliance -->
Adherence to `@docs/dev/process/workflow.md` is mandatory.
- **Branching**: No direct commits to `master` or `dev`. Work must be done in `feature/` or `fix/` branches.
- **Verification**: `python build.py --action test --test-label unit` must pass before any commit. Integration/E2E tests need not pass unless required.
- **Planning**: Significant changes require a Plan and Task breakdown in `.specify/` before coding.

## Project Constraints

### Technical Environment
- **Platforms**: Windows x64, Linux x64.
- **Build System**: Python-based (`build.py`) wrapping CMake/Ninja.
- **Toolchain**: MSVC (Windows), GCC/Clang (Linux).
- **Hardware Acceleration**: CUDA, TensorRT support is core to the value proposition.

### Operational Constraints
- **Execution**: Binaries must be executed from their output directory (e.g., `build/bin/linux-x64-debug`) to ensure relative resource paths resolve correctly.
- **Filesystem**: Recursive deletion of the `build` directory is forbidden to preserve TensorRT caches.
- **Hallucinations**: Reference only files that actually exist. Verify paths before reading/writing.

## Development Workflow

### Feature Lifecycle
1. **Plan**: Generate a plan using `/speckit.plan` or manually in `.specify/`.
2. **Tasking**: Break down the plan into atomic tasks using `/speckit.tasks`.
3. **Execution**:
   - Pick a task.
   - Write a test (Red).
   - Implement (Green).
   - Refactor.
   - Mark task complete.
4. **Verification**: Run full test suite.
5. **Merge**: Submit PR (or merge to dev if acting as solo agent).

## Governance

This Constitution supersedes all other practice guides.
- **Amendments**: Require explicit user instruction and must be recorded in the Sync Impact Report.
- **Versioning**: Follows Semantic Versioning. Major changes to principles require a Major version bump.
- **Compliance**: All PRs and Agent sessions must verify compliance with these principles.

**Version**: 1.1.0 | **Ratified**: 2026-02-15 | **Last Amended**: 2026-02-15
