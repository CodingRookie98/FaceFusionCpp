#!/usr/bin/env python3
import argparse
import sys
import subprocess
import json
from pathlib import Path
from typing import Optional

def get_video_duration(path: Path) -> float:
    try:
        cmd = [
            "ffprobe", 
            "-v", "error", 
            "-show_entries", "format=duration", 
            "-of", "default=noprint_wrappers=1:nokey=1", 
            str(path)
        ]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        return float(result.stdout.strip())
    except Exception as e:
        print(f"[WARN] Could not get video duration: {e}")
        return 0.0

def verify_file(path: Path, is_video: bool = False) -> bool:
    if not path.exists():
        print(f"[FAIL] Output file not found: {path}")
        return False
    
    if path.stat().st_size == 0:
        print(f"[FAIL] Output file is empty: {path}")
        return False
        
    print(f"[PASS] File exists: {path} ({path.stat().st_size} bytes)")
    
    if is_video:
        duration = get_video_duration(path)
        if duration > 0:
             print(f"[PASS] Video duration: {duration:.2f}s")
        else:
             print(f"[WARN] Video duration verification failed or 0s")
             
    return True

def main():
    parser = argparse.ArgumentParser(description="Verify E2E Output")
    parser.add_argument("output_path", type=Path, help="Path to the output file")
    parser.add_argument("--type", choices=["image", "video", "auto"], default="auto", help="File type")
    
    args = parser.parse_args()
    
    file_type = args.type
    if file_type == "auto":
        ext = args.output_path.suffix.lower()
        if ext in [".mp4", ".avi", ".mov", ".mkv"]:
            file_type = "video"
        else:
            file_type = "image"
            
    success = verify_file(args.output_path, is_video=(file_type == "video"))
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
