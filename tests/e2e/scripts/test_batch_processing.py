#!/usr/bin/env python3
import sys
import subprocess
import time
from pathlib import Path
from typing import Optional

def find_executable() -> Optional[Path]:
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parents[2]
    
    candidates = list(project_root.glob("build/**/FaceFusionCpp")) + \
                 list(project_root.glob("build/**/FaceFusionCpp.exe"))
                 
    candidates = [p for p in candidates if p.is_file()]
                 
    if not candidates:
        return None
        
    candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return candidates[0]

def main():
    executable = find_executable()
    if not executable or not executable.exists():
        print("Error: Executable not found. Please build the project first.")
        sys.exit(1)
        
    script_dir = Path(__file__).resolve().parent
    config_path = script_dir.parent / "configs" / "e2e_image_batch.yaml"
    
    if not config_path.exists():
        print(f"Error: Config not found at {config_path}")
        sys.exit(1)
        
    print(f"Running batch processing test with: {executable}")
    print(f"Config: {config_path}")
    
    cmd = [str(executable), "--task-config", str(config_path)]
    work_dir = executable.parent
    
    start_time = time.time()
    try:
        result = subprocess.run(
            cmd,
            cwd=str(work_dir),
            text=True,
            capture_output=True
        )
        duration = time.time() - start_time
        
        if result.returncode != 0:
            print(f"FAIL: Executable returned non-zero exit code {result.returncode}")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            sys.exit(1)
            
        output_dir = work_dir / "output" / "test" / "e2e" / "e2e_image_batch"
        expected_files = [
            "pipeline_runner_image_batch_output_woman.jpg",
            "pipeline_runner_image_batch_output_barbara.jpg"
        ]
        
        missing_files = []
        for filename in expected_files:
            file_path = output_dir / filename
            if not file_path.exists():
                missing_files.append(filename)
        
        if missing_files:
            print(f"FAIL: Missing output files: {', '.join(missing_files)}")
            print(f"Checked directory: {output_dir}")
            sys.exit(1)
            
        print(f"PASS: All expected output files exist. (Duration: {duration:.2f}s)")
        sys.exit(0)
        
    except Exception as e:
        print(f"FAIL: Exception occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
