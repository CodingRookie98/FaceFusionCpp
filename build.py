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


def ensure_configured(cmake_exe, preset, env, project_root, extra_args=None):
    build_dir_name = preset
    if (
        platform.system() == "Linux"
        and preset.startswith("linux-")
        and "x64" not in preset
    ):
        build_dir_name = preset.replace("linux-", "linux-x64-")

    build_dir = project_root / "build" / build_dir_name

    if not (build_dir / "CMakeCache.txt").exists():
        log(
            f"Build directory {build_dir} not configured. Running configure...",
            "warning",
        )
        run_configure(cmake_exe, preset, env, project_root, extra_args)


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
        exe_name = "ffc.exe" if platform.system() == "Windows" else "ffc"
        # Determine the bin directory based on the preset name (which corresponds to the build/bin/{preset} structure)
        bin_dir_name = build_dir.name
        executable = project_root / "build" / "bin" / bin_dir_name / exe_name
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


def run_web_build(project_root):
    """Build the frontend (web/) and sync static assets to assets/web/."""
    log("\n=== Building Web Frontend (web/) ===", "info")
    web_dir = project_root / "web"
    if not (web_dir / "package.json").exists():
        log("web/ not found, skipping frontend build", "warning")
        return

    npm_exe = "npm.cmd" if platform.system() == "Windows" else "npm"
    if not shutil.which(npm_exe) and not shutil.which("npm"):
        log("npm not found in PATH, skipping frontend build", "warning")
        return

    web_env = os.environ.copy()
    web_env.pop("NODE_ENV", None)

    if not (web_dir / "node_modules").exists():
        log("Installing frontend dependencies (npm ci)...", "info")
        run_command([npm_exe, "ci", "--include=dev"], env=web_env, cwd=web_dir)

    log("Building frontend (npm run build)...", "info")
    run_command([npm_exe, "run", "build"], env=web_env, cwd=web_dir)

    dist_dir = web_dir / "dist"
    if not dist_dir.exists():
        log("Frontend build produced no dist/ directory", "error")
        sys.exit(1)

    target = project_root / "assets" / "web"
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(dist_dir, target)
    log(f"Web assets synced to {target}", "success")


def _is_port_in_use(host, port):
    import socket

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(0.5)
        return s.connect_ex((host, port)) == 0


def _wait_for_health(host, port, timeout=30):
    import time
    import urllib.request

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with urllib.request.urlopen(
                f"http://{host}:{port}/api/health", timeout=1
            ) as resp:
                if resp.status == 200:
                    return True
        except OSError:
            time.sleep(0.5)
    return False


def run_dev(project_root, preset, env, web_port):
    """Start the C++ web backend + Vite dev server for local development.

    Launches `ffc --web` in the build bin dir (background), waits for
    /api/health, then starts `vite dev` in the foreground. The backend is
    terminated when vite exits (Ctrl-C or otherwise).
    """
    log("\n=== Action: dev ===", "info")

    exe_name = "ffc.exe" if platform.system() == "Windows" else "ffc"
    bin_dir_name = preset
    if (
        platform.system() == "Linux"
        and preset.startswith("linux-")
        and "x64" not in preset
    ):
        bin_dir_name = preset.replace("linux-", "linux-x64-")
    bin_dir = project_root / "build" / "bin" / bin_dir_name
    exe = bin_dir / exe_name

    if not exe.exists():
        log(
            f"Executable not found at {exe}. Run 'python build.py --action build' first.",
            "error",
        )
        sys.exit(1)

    host = "127.0.0.1"
    if _is_port_in_use(host, web_port):
        log(
            f"Port {host}:{web_port} is already in use. "
            "A previous `ffc --web` instance may still be running; stop it first "
            f"(e.g. `ss -tlnp | grep {web_port}` / Task Manager) to avoid "
            "hitting stale code on the old instance.",
            "error",
        )
        sys.exit(1)

    # NODE_ENV=production makes npm skip devDependencies; neutralize for dev.
    web_env = env.copy()
    web_env.pop("NODE_ENV", None)
    # Keep Vite's proxy in sync with the backend port/host (vite.config.ts reads these).
    web_env["FFC_WEB_PORT"] = str(web_port)
    web_env["FFC_WEB_HOST"] = host

    backend = subprocess.Popen(
        [str(exe), "--web", "--web-port", str(web_port), "--web-host", host],
        cwd=str(bin_dir),
        env=web_env,
    )
    try:
        if not _wait_for_health(host, web_port):
            log(
                f"Backend did not become ready on {host}:{web_port} within timeout.",
                "error",
            )
            sys.exit(1)
        log(f"Backend ready: http://{host}:{web_port}/ (Ctrl-C to stop)", "success")

        web_dir = project_root / "web"
        if not (web_dir / "package.json").exists():
            log("web/ not found; cannot start Vite dev server", "error")
            sys.exit(1)
        log("Starting Vite dev server...", "info")
        run_command(["npm", "run", "dev"], env=web_env, cwd=web_dir, exit_on_error=False)
    finally:
        backend.terminate()
        try:
            backend.wait(timeout=5)
        except subprocess.TimeoutExpired:
            backend.kill()
        log("Backend stopped.", "info")


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
        choices=["configure", "build", "test", "install", "package", "web", "dev"],
        default="build",
        help="Action to perform (default: build)",
    )
    parser.add_argument(
        "--web-port",
        type=int,
        default=8000,
        help="Port for the dev web backend (action: dev; also honored by Vite via FFC_WEB_PORT)",
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
        "--no-web",
        "--skip-web",
        action="store_true",
        dest="no_web",
        help="Skip building the frontend (web/) during build/package actions",
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
    parser.add_argument(
        "--enable-gpu",
        action="store_true",
        help="Enable GPU support (activates FACEFUSION_ENABLE_GPU=ON)",
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
    if (
        platform.system() == "Linux"
        and preset.startswith("linux-")
        and "x64" not in preset
    ):
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
        extra_cmake_args.extend(
            ["-DVCPKG_MANIFEST_FEATURES=test", "-DBUILD_TESTING=ON"]
        )
    else:
        extra_cmake_args.append("-DBUILD_TESTING=OFF")

    if args.enable_gpu:
        extra_cmake_args.append("-DFACEFUSION_ENABLE_GPU=ON")

    # Action Dispatcher
    if args.action == "configure":
        run_configure(cmake_exe, preset, env, project_root, extra_cmake_args)

    elif args.action == "build":
        ensure_configured(cmake_exe, preset, env, project_root, extra_cmake_args)
        run_build(cmake_exe, preset, args.target, jobs, env, project_root)
        if not args.no_web:
            run_web_build(project_root)

    elif args.action == "test":
        if not args.no_build:
            ensure_configured(cmake_exe, preset, env, project_root, extra_cmake_args)
            run_build(cmake_exe, preset, args.target, jobs, env, project_root)
        else:
            log("Skipping build step as requested.", "info")

        # Determine regex for test
        regex = args.test_regex
        if not regex and args.target != "all":
            regex = args.target

        run_test(
            ctest_exe, preset, regex, args.test_label, env, project_root, build_dir
        )

    elif args.action == "install":
        ensure_configured(cmake_exe, preset, env, project_root, extra_cmake_args)
        run_install(cmake_exe, build_dir, env, project_root)

    elif args.action == "package":
        # Ensure configured and built before packaging
        ensure_configured(cmake_exe, preset, env, project_root, extra_cmake_args)

        log("Running build before packaging...", "info")
        run_build(cmake_exe, preset, "all", jobs, env, project_root)
        if not args.no_web:
            run_web_build(project_root)

        run_package(cpack_exe, build_dir, env)

    elif args.action == "web":
        run_web_build(project_root)

    elif args.action == "dev":
        run_dev(project_root, preset, env, args.web_port)

    log("\nOperation completed successfully!", "success")


if __name__ == "__main__":
    main()
