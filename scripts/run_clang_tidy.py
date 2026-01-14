#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
from pathlib import Path
import math

# Add script directory to sys.path
script_dir = Path(__file__).parent.resolve()
sys.path.append(str(script_dir.parent))


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

    # 1. Check for clang-tidy
    clang_tidy = shutil.which("clang-tidy")
    if not clang_tidy:
        log("Error: clang-tidy executable not found in PATH.", "error")
        log(
            "Please install LLVM/Clang and ensure bin directory is in your PATH.",
            "info",
        )
        sys.exit(1)

    # 2. Find build directory with compile_commands.json
    build_dir = project_root / "build"
    compile_commands_path = None
    build_path_used = None

    if build_dir.exists():
        # Look in subdirectories
        for child in build_dir.iterdir():
            if child.is_dir() and (child / "compile_commands.json").exists():
                compile_commands_path = child / "compile_commands.json"
                build_path_used = child
                break

    if not compile_commands_path:
        log("Error: compile_commands.json not found in any build directory.", "error")
        log(
            "Please run configuration first (e.g., python build.py --action configure).",
            "info",
        )
        sys.exit(1)

    log(f"Using build directory: {build_path_used}", "info")
    log(f"Using clang-tidy: {clang_tidy}", "info")

    # 3. Scan files
    # clang-tidy typically runs on implementation files
    dirs_to_scan = ["src"]
    # Excluding .ixx/.cppm for MSVC as clang-tidy often struggles with MSVC modules commands
    extensions = {".cpp", ".cc", ".c"}

    files_to_check = []

    for d in dirs_to_scan:
        path = project_root / d
        if path.exists():
            log(f"Scanning directory: {d}", "success")
            for ext in extensions:
                files_to_check.extend(path.rglob(f"*{ext}"))

    if not files_to_check:
        log("No files found to check.", "warning")
        sys.exit(0)

    log(f"Found {len(files_to_check)} files. Collecting for batch analysis...", "info")

    # 4. Run clang-tidy in batches
    # Windows command line limit is 32k chars. 50 paths should be safe.
    batch_size = 50
    files_to_check = [str(f) for f in files_to_check]
    total_files = len(files_to_check)

    errors_found = False

    for i in range(0, total_files, batch_size):
        batch = files_to_check[i : i + batch_size]
        end_idx = min(i + batch_size, total_files)
        log(f"Checking batch ({i + 1} to {end_idx})...", "info")

        cmd = [clang_tidy, "-p", str(build_path_used)] + batch

        try:
            # We allow it to fail (non-zero exit) if it finds issues, but we want to capture that
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError:
            log("Warnings or errors found in batch.", "warning")
            errors_found = True

    if errors_found:
        log("Analysis complete with issues.", "warning")
        # Don't exit 1 here necessarily unless we want to block builds.
        # Usually static analysis issues are warnings.
        # But for strict CI, might want exit 1.
        # The PS1 script didn't exit 1 at the end, just printed warnings.
    else:
        log("Analysis complete.", "success")


if __name__ == "__main__":
    main()
