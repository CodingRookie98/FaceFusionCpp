#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
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
            for child in build_dir.iterdir():
                if child.is_dir() and (child / "compile_commands.json").exists():
                    compile_commands_path = child / "compile_commands.json"
                    build_path_used = child
                    break

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

    # 3. Clang-Tidy (Simplified)
    if clang_tidy and files_to_tidy:
        if build_path_used:
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
