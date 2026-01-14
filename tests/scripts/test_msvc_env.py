import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from scripts.utils.msvc import get_msvc_env


def main():
    print("Testing MSVC Environment Loading...")
    env = get_msvc_env()

    # Check for critical variables
    # Note: On Windows keys are case-insensitive, but in the dict they are case-preserving.
    # We should search case-insensitively.

    path_key = next((k for k in env if k.upper() == "PATH"), None)
    include_key = next((k for k in env if k.upper() == "INCLUDE"), None)

    if not path_key:
        print("FAILURE: PATH variable not found in environment.")
        sys.exit(1)

    path_val = env[path_key]
    if "cl.exe" in path_val or "CL.exe" in path_val or "cl.exe" in path_val.lower():
        # Simple string check might fail if cl.exe isn't literally in the path string but in a dir in the path.
        # But usually vcvarsall adds the bin folder to PATH.
        # Let's check if we can find cl.exe via shutil.which or just check if 'Visual Studio' is in PATH.
        pass

    # Better check: try to find cl.exe
    import shutil

    cl_path = shutil.which("cl.exe", path=path_val)

    if cl_path:
        print(f"SUCCESS: Found cl.exe at {cl_path}")
    else:
        print("FAILURE: cl.exe not found in the loaded PATH.")
        # Debug: print PATH
        # print(f"PATH: {path_val}")
        sys.exit(1)

    if include_key and env[include_key]:
        print(f"SUCCESS: INCLUDE path set.")
    else:
        print(
            "WARNING: INCLUDE variable missing or empty (might be okay if using clang-cl)."
        )

    print("MSVC Environment loaded successfully.")


if __name__ == "__main__":
    main()
