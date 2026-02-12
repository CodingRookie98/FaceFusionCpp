# Contributing to FaceFusionCpp

Thank you for your interest in contributing! We welcome bug reports, feature requests, and code contributions.

## 1. Code of Conduct
We expect all contributors to be respectful and inclusive. Harassment or abusive behavior will not be tolerated.

---

## 2. Getting Started

1.  **Fork** the repository on GitHub.
2.  **Clone** your fork locally:
    ```bash
    git clone https://github.com/YOUR-USERNAME/faceFusionCpp.git
    cd faceFusionCpp
    ```
3.  **Set up** your development environment. See the [Build Guide](build_guide.md).

---

## 3. Development Workflow

### 3.1 Branching Strategy
*   **Never** push directly to `main` (or `master`, `linux/dev`).
*   Create a new branch for your work:
    *   Features: `feature/short-description` (e.g., `feature/add-video-support`)
    *   Fixes: `fix/issue-number-description` (e.g., `fix/123-memory-leak`)

### 3.2 Test-Driven Development (TDD) - **MANDATORY**
We strictly follow TDD. You **must** write tests before implementing features.
1.  **Red**: Write a failing test case in `tests/`.
2.  **Green**: Write the minimum code to pass the test.
3.  **Refactor**: Clean up the code while keeping tests green.

### 3.3 Coding Style
*   **C++ Standard**: C++20.
*   **Format**: We use `clang-format`. Run:
    ```bash
    python scripts/format_code.py
    ```
*   **Linting**: We use `clang-tidy`. Ensure no warnings before submitting.

---

## 4. Pull Request Process

1.  **Sync**: Ensure your branch is up to date with the upstream `main`.
2.  **Test**: Run the full test suite locally:
    ```bash
    python build.py --action test
    ```
    **PRs with failing tests will be rejected.**
3.  **Commit**: Use [Conventional Commits](https://www.conventionalcommits.org/):
    *   `feat: add face enhancer`
    *   `fix: resolve crash on exit`
    *   `docs: update readme`
    *   `refactor: optimize pipeline`
4.  **Push**: Push your branch to your fork.
5.  **Create PR**: Open a Pull Request on GitHub.
    *   Describe your changes clearly.
    *   Link to related issues (e.g., "Closes #123").

---

## 5. Review Process
*   A maintainer will review your code.
*   Address any feedback or requested changes.
*   Once approved and CI passes, your code will be merged.

Thank you for helping make FaceFusionCpp better!
