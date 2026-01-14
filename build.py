#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
import platform
import shutil
from pathlib import Path

# Add script directory to sys.path to allow importing from scripts/
script_dir = Path(__file__).parent.resolve()
sys.path.append(str(script_dir))

from scripts.utils.msvc import get_msvc_env


def log(message, level="info"):
    colors = {
        "info": "\033[96m",  # Cyan
        "success": "\033[92m",  # Green
        "warning": "\033[93m",  # Yellow
        "error": "\033[91m",  # Red
        "reset": "\033[0m",
    }
    # Fallback for Windows cmd if ANSI not supported (though Win10+ supports it)
    if platform.system() == "Windows" and not os.environ.get("WT_SESSION"):
        # Simple check, might not cover all cases but good enough
        pass

    print(f"{colors.get(level, colors['reset'])}{message}{colors['reset']}")


def get_cmake_preset(config, os_name):
    """
    Determine the CMake preset based on configuration and OS.
    Using naming convention: [os/compiler]-[arch]-[config]
    e.g., msvc-x64-debug, linux-x64-release
    """
    config = config.lower()

    if os_name == "Windows":
        return f"msvc-x64-{config}"
    elif os_name == "Linux":
        return f"linux-{config}"  # Assuming linux-release / linux-debug in presets
    elif os_name == "Darwin":
        return f"macos-{config}"  # Assuming macos presets exist
    else:
        return f"default-{config}"


def run_command(cmd, env=None, cwd=None, check=True):
    cmd_str = " ".join(cmd)
    log(f"Executing: {cmd_str}", "info")
    try:
        subprocess.run(cmd, env=env, cwd=cwd, check=check)
    except subprocess.CalledProcessError as e:
        log(f"Command failed with exit code {e.returncode}", "error")
        sys.exit(e.returncode)


def main():
    parser = argparse.ArgumentParser(
        description="Cross-platform build script for FaceFusionCpp"
    )
    parser.add_argument(
        "--config",
        choices=["Debug", "Release"],
        default="Debug",
        help="Build configuration",
    )
    parser.add_argument("--target", default="all", help="Build target")
    parser.add_argument(
        "--action",
        choices=["configure", "build", "test", "install", "package", "both"],
        default="both",
        help="Action to perform",
    )
    parser.add_argument("--preset", help="Override CMake preset")
    parser.add_argument(
        "--clean", action="store_true", help="Clean build directory before starting"
    )

    args = parser.parse_args()

    project_root = script_dir
    os_name = platform.system()

    # 1. Environment Setup
    env = os.environ.copy()
    if os_name == "Windows":
        log("Detecting MSVC environment...", "info")
        env = get_msvc_env()

    # 2. Determine Preset
    preset = args.preset if args.preset else get_cmake_preset(args.config, os_name)
    log(f"Configuration: {args.config}", "info")
    log(f"Preset: {preset}", "info")

    # 3. Clean if requested
    build_dir = project_root / "build" / preset
    if args.clean and build_dir.exists():
        log(f"Cleaning build directory: {build_dir}", "warning")
        shutil.rmtree(build_dir)

    # 4. Actions
    cmake_exe = "cmake"  # Assumed to be in PATH

    actions = []
    if args.action == "both":
        actions = ["configure", "build"]
    else:
        actions = [args.action]

    for action in actions:
        log(f"\n=== Action: {action} ===", "info")

        if action == "configure":
            cmd = [cmake_exe, "--preset", preset]
            run_command(cmd, env=env, cwd=project_root)

        elif action == "build":
            cmd = [cmake_exe, "--build", "--preset", preset]
            if args.target != "all":
                cmd.extend(["--target", args.target])
            run_command(cmd, env=env, cwd=project_root)

        elif action == "test":
            # Run build first if not explicitly skipped? user usually manages this.
            # But let's follow ctest preset
            ctest_exe = "ctest"
            cmd = [ctest_exe, "--preset", preset]
            run_command(cmd, env=env, cwd=project_root)

        elif action == "install":
            cmd = [cmake_exe, "--install", str(build_dir)]
            run_command(cmd, env=env, cwd=project_root)

        elif action == "package":
            cpack_exe = "cpack"
            # CPack usually needs to run from build dir or specified config
            # --config build/preset/CPackConfig.cmake
            cpack_config = build_dir / "CPackConfig.cmake"
            if cpack_config.exists():
                cmd = [cpack_exe, "--config", str(cpack_config), "-V"]
                run_command(cmd, env=env, cwd=build_dir)  # Run in build dir usually
            else:
                log(f"CPackConfig.cmake not found in {build_dir}", "error")
                sys.exit(1)

    log("\nOperation completed successfully!", "success")


if __name__ == "__main__":
    main()
