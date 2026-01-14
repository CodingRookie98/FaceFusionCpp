import os
import subprocess
import sys
import platform
from pathlib import Path


def get_msvc_env():
    """
    Returns a dictionary of environment variables with MSVC tools (cl.exe, link.exe, etc.)
    configured, similar to loading vcvarsall.bat.
    On non-Windows platforms, returns the current environment unchanged.
    """
    if platform.system() != "Windows":
        return os.environ.copy()

    # Find Visual Studio installation using vswhere
    vswhere_path = os.path.join(
        os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)"),
        "Microsoft Visual Studio/Installer/vswhere.exe",
    )

    if not os.path.exists(vswhere_path):
        print(
            "Warning: vswhere.exe not found. MSVC environment might not be loaded correctly."
        )
        return os.environ.copy()

    try:
        # -latest: Get the newest version
        # -products *: Get any product (Community, Pro, Enterprise, BuildTools)
        # -requires ...: Ensure C++ tools are installed
        # -property installationPath: Only return the path
        cmd = [
            vswhere_path,
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationPath",
        ]
        vs_path = subprocess.check_output(cmd, encoding="utf-8").strip()
    except subprocess.CalledProcessError:
        print("Warning: Failed to find Visual Studio installation via vswhere.")
        return os.environ.copy()

    if not vs_path:
        print("Warning: No suitable Visual Studio installation found.")
        return os.environ.copy()

    # Path to vcvarsall.bat
    vcvarsall_path = os.path.join(vs_path, "VC/Auxiliary/Build/vcvarsall.bat")
    if not os.path.exists(vcvarsall_path):
        print(f"Warning: vcvarsall.bat not found at {vcvarsall_path}")
        return os.environ.copy()

    # Run vcvarsall.bat and dump environment variables
    # We use 'set' to list all variables after loading the environment
    cmd = f'"{vcvarsall_path}" x64 && set'

    try:
        output = subprocess.check_output(
            cmd, shell=True, encoding="mbcs", stderr=subprocess.STDOUT
        )
    except subprocess.CalledProcessError as e:
        print(f"Error: Failed to load MSVC environment: {e.output}")
        return os.environ.copy()

    # Parse environment variables
    new_env = {}
    for line in output.splitlines():
        if "=" in line:
            key, _, value = line.partition("=")
            new_env[key.upper()] = (
                value  # Windows env vars are case-insensitive, normalizing to upper keys for consistency might be safer but os.environ preserves case. Let's trust the output.
            )
            # Actually, standard os.environ keys are preserved case, but lookups are case-insensitive on Windows.
            # Let's just use the original key.
            new_env[key] = value

    return new_env
