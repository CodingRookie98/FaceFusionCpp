#!/usr/bin/env python3
import sys
import subprocess
import time
import signal
import os
from pathlib import Path

def main():
    if len(sys.argv) < 3:
        print("Usage: test_interrupt_resume.py <executable> <config_path>")
        sys.exit(1)

    executable = Path(sys.argv[1]).resolve()
    config_path = Path(sys.argv[2]).resolve()
    
    if not executable.exists():
        print(f"Executable not found: {executable}")
        sys.exit(1)
        
    print(f"Testing Interrupt & Resume on {config_path.name}")
    
    print("\n--- Phase 1: Start and Interrupt ---")
    start_time = time.time()
    
    proc = subprocess.Popen(
        [str(executable), "--config", str(config_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=executable.parent
    )
    
    print("Waiting 5 seconds for processing to start...")
    time.sleep(5)
    
    if proc.poll() is not None:
        print("Process exited prematurely!")
        outs, errs = proc.communicate()
        print(outs)
        print(errs)
        sys.exit(1)
        
    print("Sending SIGINT (Ctrl+C)...")
    proc.send_signal(signal.SIGINT)
    
    try:
        proc.wait(timeout=10)
        print(f"Process exited with code {proc.returncode}")
    except subprocess.TimeoutExpired:
        print("Process did not exit gracefully! Killing...")
        proc.kill()
        sys.exit(1)
        
    print("\n--- Phase 2: Resume ---")
    resume_start = time.time()
    
    proc_resume = subprocess.run(
        [str(executable), "--config", str(config_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=executable.parent
    )
    
    resume_duration = time.time() - resume_start
    
    if proc_resume.returncode != 0:
        print("Resume run failed!")
        print(proc_resume.stdout)
        print(proc_resume.stderr)
        sys.exit(1)
        
    print(f"Resume run completed in {resume_duration:.2f}s")
    print("Test PASSED")

if __name__ == "__main__":
    main()
