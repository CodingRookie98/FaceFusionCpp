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
    
    cmd = [str(executable.absolute()), "--config", str(config.absolute())]
    
    if dry_run:
        print(f"[DRY-RUN] Command: {' '.join(cmd)}")
        return True

    start_time = time.time()
    try:
        # Use subprocess.run to execute the command.
        # Capture output is False to stream stdout/stderr to the console (User Requirement: preserve full output)
        result = subprocess.run(
            cmd,
            text=True,
            capture_output=False,
            timeout=600
        )
        duration = time.time() - start_time
        
        if result.returncode == 0:
            print(f"\n[PASS] {config.name} (Duration: {duration:.2f}s)")
            return True
        else:
            print(f"\n[FAIL] {config.name} (Exit Code: {result.returncode})")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"\n[FAIL] {config.name} (Timeout after 600s)")
        return False
    except Exception as e:
        print(f"\n[FAIL] {config.name} (Exception: {e})")
        return False

def main():
    parser = argparse.ArgumentParser(description="Run FaceFusionCpp E2E Tests")
    
    script_dir = Path(__file__).resolve().parent
    default_config_dir = script_dir.parent / "configs"
    
    parser.add_argument("--config-dir", type=Path, default=default_config_dir, help="Directory containing config files")
    parser.add_argument("--executable", type=Path, default=Path("./FaceFusionCpp"), help="Path to the executable")
    parser.add_argument("--filter", type=str, help="Filter tests by name")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without executing")
    
    args = parser.parse_args()
    
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
        if run_test(args.executable, config, args.dry_run):
            success_count += 1
        else:
            fail_count += 1
            
    print("-" * 40)
    print(f"Total: {len(configs)}, Passed: {success_count}, Failed: {fail_count}")
    
    sys.exit(0 if fail_count == 0 else 1)

if __name__ == "__main__":
    main()
