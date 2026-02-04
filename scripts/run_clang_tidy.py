#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
from pathlib import Path
import math
import concurrent.futures
import time

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

    # 1. Check for clang-tidy
    clang_tidy = shutil.which("clang-tidy")
    if not clang_tidy:
        # Try versioned binaries
        for version in range(21, 10, -1):
            name = f"clang-tidy-{version}"
            clang_tidy = shutil.which(name)
            if clang_tidy:
                break

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
    extensions = {".cpp", ".cc", ".c"}

    # Check if using MSVC - if so, skip module files (.ixx, .cppm) as clang-tidy often struggles with MSVC modules
    is_msvc = is_msvc_build(compile_commands_path)

    if not is_msvc:
        extensions.update({".ixx", ".cppm"})
    else:
        log(
            "Detected MSVC compiler usage. Skipping .ixx/.cppm files for static analysis.",
            "info",
        )

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

    log(f"Found {len(files_to_check)} files. Starting parallel analysis...", "info")

    # 4. Run clang-tidy in parallel
    # Use number of CPU cores for parallelism
    max_workers = os.cpu_count() or 4
    files_to_check = [str(f) for f in files_to_check]
    total_files = len(files_to_check)

    errors_found = False
    completed_count = 0
    start_time = time.time()

    def run_tidy_on_file(file_path):
        cmd = [clang_tidy, "-p", str(build_path_used), file_path]
        try:
            # Capture output to avoid interleaving in terminal
            result = subprocess.run(cmd, capture_output=True, text=True)
            return file_path, result.returncode, result.stdout, result.stderr
        except Exception as e:
            return file_path, 1, "", str(e)

    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
        future_to_file = {executor.submit(run_tidy_on_file, f): f for f in files_to_check}

        for future in concurrent.futures.as_completed(future_to_file):
            file_path, returncode, stdout, stderr = future.result()
            completed_count += 1

            # Show progress every 10 files or at the end
            if completed_count % 10 == 0 or completed_count == total_files:
                elapsed = time.time() - start_time
                log(f"Progress: {completed_count}/{total_files} files checked ({elapsed:.1f}s)...", "info")

            if returncode != 0 or stdout or stderr:
                if stdout or stderr:
                    log(f"\n[Issue in {file_path}]", "warning")
                    if stdout:
                        print(stdout)
                    if stderr:
                        print(stderr)
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
