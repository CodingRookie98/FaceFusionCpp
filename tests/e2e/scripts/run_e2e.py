#!/usr/bin/env python3
import argparse
import sys
import subprocess
import time
from pathlib import Path
from typing import List

def find_configs(config_dir: Path) -> List[Path]:
    if not config_dir.exists():
        return []
    return sorted(list(config_dir.glob("*.yaml")))

def run_test(executable: Path, config: Path, dry_run: bool = False) -> bool:
    print(f"\n{'='*60}")
    print(f"[TEST] Running {config.name}...")
    print(f"{'='*60}")
    
    executable = executable.resolve()
    config = config.resolve()
    
    cmd = [str(executable), "--task-config", str(config)]
    
    work_dir = executable.parent
    
    if dry_run:
        print(f"[DRY-RUN] Command: {' '.join(cmd)}")
        print(f"[DRY-RUN] Working Directory: {work_dir}")
        return True

    start_time = time.time()
    try:
        result = subprocess.run(
            cmd,
            cwd=str(work_dir),
            text=True,
            capture_output=False
        )
        duration = time.time() - start_time
        
        if result.returncode == 0:
            print(f"\n[PASS] {config.name} (Duration: {duration:.2f}s)")
            return True
        else:
            print(f"\n[FAIL] {config.name} (Exit Code: {result.returncode})")
            return False
            
    except Exception as e:
        print(f"\n[FAIL] {config.name} (Exception: {e})")
        return False

def find_executable(provided_path: Path) -> Path:
    if provided_path.exists() and provided_path.is_file():
        return provided_path
        
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parents[2]
    
    print(f"Executable not found at {provided_path}, searching in {project_root}/build/...")
    
    candidates = list(project_root.glob("build/*/bin/FaceFusionCpp")) + \
                 list(project_root.glob("build/*/bin/FaceFusionCpp.exe"))
                 
    if not candidates:
        return provided_path
        
    candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    
    best_candidate = candidates[0]
    print(f"Auto-detected executable: {best_candidate}")
    return best_candidate

def main():
    parser = argparse.ArgumentParser(description="Run FaceFusionCpp E2E Tests")
    
    script_dir = Path(__file__).resolve().parent
    default_config_dir = script_dir.parent / "configs"
    
    parser.add_argument("--config-dir", type=Path, default=default_config_dir, help="Directory containing config files")
    parser.add_argument("--executable", type=Path, default=Path("./FaceFusionCpp"), help="Path to the executable")
    parser.add_argument("--filter", type=str, help="Filter tests by name")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without executing")
    
    args = parser.parse_args()
    
    executable = find_executable(args.executable)
    if not executable.exists():
        print(f"Error: Executable not found at {executable}")
        sys.exit(1)
        
    configs = find_configs(args.config_dir)
    if args.filter:
        configs = [c for c in configs if args.filter in c.name]
        
    if not configs:
        print(f"No configs found in {args.config_dir}")
        sys.exit(1)
        
    success_count = 0
    fail_count = 0
    
    print(f"Found {len(configs)} tests.")
    
    for config in configs:
        if run_test(executable, config, args.dry_run):
            success_count += 1
        else:
            fail_count += 1
            
    print("-" * 40)
    print(f"Total: {len(configs)}, Passed: {success_count}, Failed: {fail_count}")
    
    sys.exit(0 if fail_count == 0 else 1)

if __name__ == "__main__":
    main()
