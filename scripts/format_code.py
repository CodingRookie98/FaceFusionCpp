#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
from pathlib import Path

# Add script directory to sys.path to allow importing from scripts/
script_dir = Path(__file__).parent.resolve()
sys.path.append(str(script_dir.parent))

from scripts.utils.msvc import get_msvc_env


def log(message, level="info"):
    colors = {
        "info": "\033[96m",  # Cyan
        "success": "\033[92m",  # Green
        "warning": "\033[93m",  # Yellow
        "error": "\033[91m",  # Red
        "reset": "\033[0m",
    }
    # Simple Windows check
    if sys.platform == "win32" and not os.environ.get("WT_SESSION"):
        pass

    print(f"{colors.get(level, colors['reset'])}{message}{colors['reset']}")


def main():
    project_root = script_dir.parent

    # 1. Check for clang-format
    clang_format = shutil.which("clang-format")
    if not clang_format:
        log("Error: clang-format executable not found in PATH.", "error")
        log(
            "Please install LLVM/Clang and ensure bin directory is in your PATH.",
            "info",
        )
        sys.exit(1)

    log(f"Using clang-format: {clang_format}", "info")
    log(f"Project Root: {project_root}", "info")

    # 2. Scan files
    dirs_to_scan = ["src", "facefusionCpp", "tests"]
    extensions = {".cpp", ".h", ".hpp", ".ixx", ".cppm", ".c", ".cc"}

    files_to_format = []

    for d in dirs_to_scan:
        path = project_root / d
        if path.exists():
            log(f"Scanning directory: {d}", "success")
            # rglob is recursive glob
            for ext in extensions:
                # rglob pattern is like "**/*.cpp"
                files_to_format.extend(path.rglob(f"*{ext}"))
        else:
            log(f"Directory not found: {d}", "warning")

    if not files_to_format:
        log("No files found to format.", "warning")
        sys.exit(0)

    log(f"Found {len(files_to_format)} files. Starting formatting...", "info")

    # 3. Format files
    format_errors = False
    for file_path in files_to_format:
        # log(f"Formatting: {file_path}", "info") # Too verbose?
        print(f"Formatting: {file_path}")
        try:
            # -i edits in place, -style=file uses .clang-format
            subprocess.run(
                [clang_format, "-i", "-style=file", str(file_path)], check=True
            )
        except subprocess.CalledProcessError:
            log(f"Error formatting file: {file_path}", "error")
            format_errors = True

    if format_errors:
        log("Formatting complete with errors.", "error")
        sys.exit(1)
    else:
        log("Formatting complete.", "success")


if __name__ == "__main__":
    main()
