#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
import re
from pathlib import Path

# Add script directory to sys.path
script_dir = Path(__file__).parent.resolve()
sys.path.append(str(script_dir.parent))

from scripts.utils.msvc import is_msvc_build


def log(message, level="info"):
    colors = {
        "info": "\033[96m",  # Cyan
        "success": "\033[92m",  # Green
        "warning": "\033[93m",  # Yellow
        "error": "\033[91m",  # Red
        "reset": "\033[0m",
    }
    if sys.platform == "win32" and not os.environ.get("WT_SESSION"):
        pass
    print(f"{colors.get(level, colors['reset'])}{message}{colors['reset']}")


def check_test_naming_convention(files):
    errors = []

    # Regex to match file paths starting with tests/ and filename starting with test_
    # Note: We use forward slash as git returns paths with forward slashes usually
    file_name_pattern = re.compile(r"^tests/.*/test_[^/]+\.cpp$")
    
    # Regex to match TEST macros with underscores in the second argument (TestName)
    # TEST(TestSuiteName, TestName) or TEST_F(TestSuiteName, TestName)
    test_case_pattern = re.compile(r"TEST(?:_F)?\s*\(\s*[a-zA-Z0-9_]+\s*,\s*[a-zA-Z0-9_]+_[a-zA-Z0-9_]+\s*\)")

    for f in files:
        # Normalize path separators just in case, though git usually uses /
        f_normalized = f.replace(os.sep, "/")
        
        # Check if it is a test file in tests directory
        if not f_normalized.startswith("tests/") or not f.endswith(".cpp"):
            continue

        # Skip test_support files if they are named test_support.cpp (exception)
        if "test_support.cpp" in f:
            continue

        # 1. Check file name convention
        if file_name_pattern.search(f_normalized):
            errors.append(f"File naming violation: {f} (Should not start with 'test_', use '_test.cpp' suffix)")

        # 2. Check test case naming convention
        if os.path.exists(f):
            try:
                with open(f, 'r', encoding='utf-8') as file_content:
                    content = file_content.read()
                    matches = test_case_pattern.finditer(content)
                    for match in matches:
                        # Double check to ensure it's not a false positive (e.g. commented out)
                        # For simplicity, we assume code is clean.
                        errors.append(f"Test case naming violation in {f}: '{match.group(0)}' (TestName should be PascalCase without underscores)")
            except Exception as e:
                # If file doesn't exist (deleted) or can't be read, skip
                pass

    return errors


def main():
    project_root = script_dir.parent
    os.chdir(project_root)

    log("Running pre-commit checks...", "info")

    # Check required tools
    clang_format = shutil.which("clang-format")
    if not clang_format:
        log("Error: clang-format not found in PATH.", "error")
        sys.exit(1)

    clang_tidy = shutil.which("clang-tidy")
    if not clang_tidy:
        log(
            "Warning: clang-tidy not found in PATH. Skipping static analysis.",
            "warning",
        )

    # 1. Get Staged Files
    log("Checking for staged files...", "info")
    try:
        # --diff-filter=ACMR: Added, Copied, Modified, Renamed
        result = subprocess.run(
            ["git", "diff", "--cached", "--name-only", "--diff-filter=ACMR"],
            capture_output=True,
            text=True,
            check=True,
        )
        staged_files = result.stdout.splitlines()
    except subprocess.CalledProcessError:
        log("Failed to get staged files.", "error")
        sys.exit(1)

    if not staged_files:
        log("No staged files found.", "success")
        sys.exit(0)

    # Filter for C++ files
    format_extensions = {".cpp", ".h", ".hpp", ".ixx", ".cppm", ".c", ".cc"}
    tidy_extensions = {".cpp", ".c", ".cc"}

    # Detect MSVC and optional modules support
    is_msvc = False
    compile_commands_path = None
    build_path_used = None

    # Try to find compile_commands.json to detect MSVC
    if clang_tidy:
        build_dir = project_root / "build"
        if build_dir.exists():
            # Strategy: Prefer debug builds, then any other valid build dir
            candidates = []
            for child in build_dir.iterdir():
                if child.is_dir() and (child / "compile_commands.json").exists():
                    candidates.append(child)
            
            if candidates:
                # Sort: directories with 'debug' in name come first (0), others second (1)
                # Secondary sort by name for stability
                candidates.sort(key=lambda p: (0 if "debug" in p.name.lower() else 1, p.name))
                
                build_path_used = candidates[0]
                compile_commands_path = build_path_used / "compile_commands.json"

        if compile_commands_path:
            is_msvc = is_msvc_build(compile_commands_path)

    if not is_msvc:
        tidy_extensions.update({".ixx", ".cppm"})
    else:
        log("Detected MSVC. Skipping .ixx/.cppm files for static analysis.", "info")

    files_to_format = []
    files_to_tidy = []

    for f in staged_files:
        path = Path(f)
        if path.exists():  # Should exist if ACMR, but safer to check
            if path.suffix in format_extensions:
                files_to_format.append(f)
            if path.suffix in tidy_extensions:
                files_to_tidy.append(f)

    if not files_to_format:
        log("No C++ source files to check.", "success")
        sys.exit(0)

    log(f"Found {len(files_to_format)} files to process.", "info")

    # 2. Format and Re-stage
    log("Running code formatting...", "info")
    format_errors = False

    for f in files_to_format:
        print(f"Formatting: {f}")
        try:
            subprocess.run([clang_format, "-i", "-style=file", f], check=True)
            # Re-stage
            subprocess.run(["git", "add", f], check=True)
        except subprocess.CalledProcessError:
            log(f"Failed to format: {f}", "error")
            format_errors = True

    if format_errors:
        log("Code formatting encountered errors.", "error")
        sys.exit(1)

    # 2.5 Test Naming Convention Check
    log("Checking test naming conventions...", "info")
    naming_errors = check_test_naming_convention(staged_files)
    if naming_errors:
        log("Found test naming convention violations:", "error")
        for err in naming_errors:
            print(f"  - {err}")
        log("Please fix these violations before committing.", "error")
        sys.exit(1)

    # 3. Clang-Tidy (Simplified)
    if clang_tidy and files_to_tidy:
        if is_msvc:
            log(
                "Detected MSVC. Disabling clang-tidy hook check due to C++20 Modules compatibility issues.",
                "warning",
            )
            log(
                "Please run 'python scripts/run_clang_tidy.py' manually if needed.",
                "info",
            )
        elif build_path_used:
            log(
                f"Running static analysis using build directory: {build_path_used}",
                "info",
            )
            try:
                # We can run on all files at once (assuming not too many staged files)
                # or batch them if needed. Usually staged files count is low.
                cmd = [clang_tidy, "-p", str(build_path_used)] + files_to_tidy
                subprocess.run(cmd, check=True)
            except subprocess.CalledProcessError:
                log(
                    "Static analysis found issues. Please fix them before committing.",
                    "error",
                )
                sys.exit(1)
        else:
            log(
                "Skipping static analysis (compile_commands.json not found in build directories).",
                "warning",
            )

    elif not files_to_tidy:
        log("No implementation files to static analyze.", "success")

    log("Pre-commit checks passed.", "success")
    sys.exit(0)


if __name__ == "__main__":
    main()
