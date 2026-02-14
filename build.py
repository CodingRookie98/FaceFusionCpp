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


def run_command(cmd, env=None, cwd=None, check=True, exit_on_error=True):
    cmd_str = " ".join(cmd)
    log(f"Executing: {cmd_str}", "info")
    try:
        return subprocess.run(cmd, env=env, cwd=cwd, check=check)
    except subprocess.CalledProcessError as e:
        if exit_on_error:
            log(f"Command failed with exit code {e.returncode}", "error")
            sys.exit(e.returncode)
        raise e


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
        return f"linux-{config}"
    elif os_name == "Darwin":
        return f"macos-{config}"
    else:
        return f"default-{config}"


def run_configure(cmake_exe, preset, env, project_root, extra_args=None):
    log("\n=== Action: configure ===", "info")
    cmd = [cmake_exe, "--preset", preset]
    if extra_args:
        cmd.extend(extra_args)
    run_command(cmd, env=env, cwd=project_root)


def run_build(cmake_exe, preset, target, jobs, env, project_root):
    log("\n=== Action: build ===", "info")
    cmd = [cmake_exe, "--build", "--preset", preset]
    if jobs:
        cmd.extend(["--parallel", str(jobs)])
    if target != "all":
        cmd.extend(["--target", target])

    try:
        run_command(cmd, env=env, cwd=project_root)
    except subprocess.CalledProcessError:
        log("\nBuild failed!", "error")
        # Check if it might be due to missing configuration
        build_dir_name = preset  # Simplified check, actual build dir depends on preset
        log(
            "Hint: If the build directory does not exist, run with '--action configure' first.",
            "warning",
        )
        sys.exit(1)


def run_test(ctest_exe, preset, regex, label, env, project_root, build_dir):
    log("\n=== Action: test ===", "info")

    if label == "e2e":
        exe_name = "FaceFusionCpp.exe" if platform.system() == "Windows" else "FaceFusionCpp"
        executable = build_dir / "bin" / exe_name
        e2e_script = project_root / "tests" / "e2e" / "scripts" / "run_e2e.py"

        if not executable.exists():
            log(f"Executable not found at {executable}. Please build first.", "error")
            sys.exit(1)

        cmd = [sys.executable, str(e2e_script), "--executable", str(executable)]
        if regex:
            cmd.extend(["--filter", regex])

        run_command(cmd, env=env, cwd=project_root)
        return

    cmd = [ctest_exe, "--preset", preset, "--no-tests=error"]

    # Determine test filter
    test_filter = None
    if regex:
        test_filter = regex

    if regex:
        cmd.extend(["-R", regex])

    if label:
        cmd.extend(["-L", label])
        if not test_filter:
             test_filter = f"label:{label}"

    try:
        run_command(cmd, env=env, cwd=project_root, exit_on_error=False)
    except subprocess.CalledProcessError as e:
        filter_msg = f" '{test_filter}'" if test_filter else ""
        if e.returncode == 8:
            log(f"\nNo tests matched the pattern{filter_msg}.", "warning")
            log("Check if the test target is correctly registered and named.", "info")
        else:
            log(f"Tests failed with exit code {e.returncode}", "error")
        sys.exit(e.returncode)


def run_install(cmake_exe, build_dir, env, project_root):
    log("\n=== Action: install ===", "info")
    cmd = [cmake_exe, "--install", str(build_dir)]
    run_command(cmd, env=env, cwd=project_root)


def run_package(cpack_exe, build_dir, env):
    log("\n=== Action: package ===", "info")
    cpack_config = build_dir / "CPackConfig.cmake"
    if cpack_config.exists():
        cmd = [cpack_exe, "--config", str(cpack_config), "-V"]
        run_command(cmd, env=env, cwd=build_dir)
    else:
        log(f"CPackConfig.cmake not found in {build_dir}", "error")
        sys.exit(1)


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
        choices=["configure", "build", "test", "install", "package"],
        default="build",
        help="Action to perform (default: build)",
    )
    parser.add_argument("--preset", help="Override CMake preset")
    parser.add_argument(
        "--clean", action="store_true", help="Clean build directory before starting"
    )
    parser.add_argument(
        "--test-regex",
        help="Regex for tests to run (passed to ctest -R). If specified, --target is ignored for testing.",
    )
    parser.add_argument(
        "--test-label",
        help="Label for tests to run (passed to ctest -L).",
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Skip the build step when action is test",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        help="Number of parallel build jobs",
    )
    parser.add_argument(
        "--enable-tests",
        action="store_true",
        help="Enable building tests (activates 'test' feature in vcpkg and BUILD_TESTING=ON)",
    )

    args = parser.parse_args()

    project_root = script_dir
    os_name = platform.system()

    # 1. Environment Setup
    env = os.environ.copy()
    if os_name == "Windows":
        log("Detecting MSVC environment...", "info")
        env = get_msvc_env()

    # Determine parallel jobs
    jobs = args.jobs
    if jobs is None:
        # Default to max available cores
        jobs = os.cpu_count() or 1

    log(f"Parallel jobs: {jobs}", "info")

    # 2. Determine Preset
    preset = args.preset if args.preset else get_cmake_preset(args.config, os_name)
    log(f"Configuration: {args.config}", "info")
    log(f"Preset: {preset}", "info")

    # 3. Clean if requested
    # We need to guess the build dir path based on preset convention
    # This might be fragile if preset defines a different binaryDir, but common convention holds.
    build_dir_name = preset
    if platform.system() == "Linux" and preset.startswith("linux-") and "x64" not in preset:
        # Fix for linux-debug -> linux-x64-debug convention in CMakePresets.json
        build_dir_name = preset.replace("linux-", "linux-x64-")

    build_dir = project_root / "build" / build_dir_name
    if args.clean and build_dir.exists():
        log(f"Cleaning build directory: {build_dir}", "warning")
        shutil.rmtree(build_dir)

    # 4. Actions
    cmake_exe = "cmake"
    ctest_exe = "ctest"
    cpack_exe = "cpack"

    # Determine CMake arguments
    extra_cmake_args = []
    if args.enable_tests or args.action == "test":
        extra_cmake_args.extend(["-DVCPKG_MANIFEST_FEATURES=test", "-DBUILD_TESTING=ON"])
    else:
        extra_cmake_args.append("-DBUILD_TESTING=OFF")

    # Action Dispatcher
    if args.action == "configure":
        run_configure(cmake_exe, preset, env, project_root, extra_cmake_args)

    elif args.action == "build":
        run_build(cmake_exe, preset, args.target, jobs, env, project_root)

    elif args.action == "test":
        if not args.no_build:
            run_build(cmake_exe, preset, args.target, jobs, env, project_root)
        else:
            log("Skipping build step as requested.", "info")

        # Determine regex for test
        regex = args.test_regex
        if not regex and args.target != "all":
            regex = args.target

        run_test(ctest_exe, preset, regex, args.test_label, env, project_root, build_dir)

    elif args.action == "install":
        run_install(cmake_exe, build_dir, env, project_root)

    elif args.action == "package":
        # Ensure configured and built before packaging
        if not build_dir.exists() or not (build_dir / "CMakeCache.txt").exists():
            log("Configuration not found. Running configure...", "info")
            run_configure(cmake_exe, preset, env, project_root, extra_cmake_args)
            
        log("Running build before packaging...", "info")
        run_build(cmake_exe, preset, "all", jobs, env, project_root)

        run_package(cpack_exe, build_dir, env)

    log("\nOperation completed successfully!", "success")


if __name__ == "__main__":
    main()
