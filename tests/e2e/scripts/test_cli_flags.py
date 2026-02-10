#!/usr/bin/env python3
import sys
import subprocess
import shutil
from pathlib import Path

def run_cmd(cmd, description, cwd=None):
    print(f"\n[TEST] {description}")
    print(f"CMD: {' '.join(cmd)}")
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=cwd)
    if result.returncode == 0:
        print("[PASS]")
        return True
    else:
        print(f"[FAIL] Exit Code: {result.returncode}")
        print(result.stdout)
        print(result.stderr)
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: test_cli_flags.py <executable>")
        sys.exit(1)
        
    executable = Path(sys.argv[1]).resolve()
    if not executable.exists():
        print(f"Executable not found: {executable}")
        sys.exit(1)
    
    cwd = executable.parent

    if not run_cmd([str(executable), "--system-check"], "System Check", cwd=cwd):
        sys.exit(1)
        
    source = "../../../assets/standard_face_test_images/lenna.bmp"
    target = "../../../assets/standard_face_test_images/girl.bmp"
    output_dir = "tests_output/cli_shortcut/"
    
    cmd = [
        str(executable),
        "-s", source,
        "-t", target,
        "-o", output_dir,
        "--headless"
    ]
    
    if run_cmd(cmd, "Shortcut Mode (-s -t -o)", cwd=cwd):
        out_path = cwd / output_dir
        if out_path.exists() and any(out_path.iterdir()):
             print("[PASS] Output file created")
        else:
             print(f"[FAIL] Output file not found in {out_path}")
    else:
        sys.exit(1)
        
    print("\nAll CLI tests passed!")

if __name__ == "__main__":
    main()
