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
    print(f"[TEST] Running {config.name}...")
    
    cmd = [str(executable.absolute()), "--config", str(config.absolute())]
    
    if dry_run:
        print(f"[DRY-RUN] Command: {' '.join(cmd)}")
        return True

    start_time = time.time()
    try:
        # Capture output to prevent cluttering console unless verbose
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=300 # 5 min timeout
        )
        duration = time.time() - start_time
        
        if result.returncode == 0:
            print(f"[PASS] {config.name} (Duration: {duration:.2f}s)")
            return True
        else:
            print(f"[FAIL] {config.name} (Exit Code: {result.returncode})")
            print("--- STDOUT ---")
            print(result.stdout)
            print("--- STDERR ---")
            print(result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        print(f"[FAIL] {config.name} (Timeout after 300s)")
        return False
    except Exception as e:
        print(f"[FAIL] {config.name} (Exception: {e})")
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
