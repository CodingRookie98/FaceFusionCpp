#!/usr/bin/env python3
import os
import sys
import subprocess
import time
import yaml
import shutil
from pathlib import Path

def find_executable():
    script_dir = Path(__file__).resolve().parent
    current = script_dir
    while current.name != "faceFusionCpp" and current.parent != current:
        current = current.parent
    project_root = current
    
    candidates = list(project_root.glob("build/bin/*/FaceFusionCpp")) + \
                 list(project_root.glob("build/bin/*/FaceFusionCpp.exe")) + \
                 list(project_root.glob("build/*/bin/FaceFusionCpp")) + \
                 list(project_root.glob("build/*/bin/FaceFusionCpp.exe"))
                 
    if not candidates:
        return None
        
    candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return candidates[0]

def test_batch_image_processing():
    executable = find_executable()
    if not executable:
        print("Error: FaceFusionCpp executable not found.")
        sys.exit(1)
        
    print(f"Using executable: {executable}")
    
    test_dir = Path(__file__).resolve().parent / "temp_batch_test"
    if test_dir.exists():
        shutil.rmtree(test_dir)
    test_dir.mkdir(parents=True, exist_ok=True)
    
    try:
        output_dir = test_dir / "output"
        output_dir.mkdir(parents=True, exist_ok=True)
        
        project_root = Path(__file__).resolve().parent
        while project_root.name != "faceFusionCpp" and project_root.parent != project_root:
            project_root = project_root.parent
            
        assets_dir = project_root / "assets" / "standard_face_test_images"
        
        source_path = assets_dir / "lenna.bmp"
        target_paths = [
            assets_dir / "woman.jpg",
            assets_dir / "barbara.bmp",
            assets_dir / "man.bmp"
        ]
        
        config = {
            "config_version": "1.0",
            "task_info": {
                "id": "batch_image_test",
                "description": "E2E test for batch image processing",
                "enable_logging": True
            },
            "io": {
                "source_paths": [str(source_path.resolve())],
                "target_paths": [str(p.resolve()) for p in target_paths],
                "output": {
                    "path": str(output_dir.resolve()),
                    "prefix": "batch_out_",
                    "suffix": "",
                    "image_format": "jpg",
                    "conflict_policy": "overwrite"
                }
            },
            "pipeline": [
                {
                    "step": "face_swapper",
                    "enabled": True,
                    "params": {
                        "model": "inswapper_128_fp16",
                        "face_selector_mode": "many"
                    }
                }
            ]
        }
        
        config_path = test_dir / "batch_test_config.yaml"
        with open(config_path, "w") as f:
            yaml.dump(config, f)
            
        print(f"Running batch processing test with {len(target_paths)} targets...")
        cmd = [str(executable), "--task-config", str(config_path)]
        
        start_time = time.time()
        result = subprocess.run(cmd, cwd=str(executable.parent), capture_output=True, text=True, timeout=600)
        duration = time.time() - start_time
        
        print(f"Execution finished in {duration:.2f}s")
        
        if result.returncode != 0:
            print(f"STDOUT: {result.stdout}")
            print(f"STDERR: {result.stderr}")
        
        assert result.returncode == 0, f"Executable failed with return code {result.returncode}"
        
        generated_files = list(output_dir.glob("*.jpg"))
        print(f"Generated files: {[f.name for f in generated_files]}")
        
        expected_count = len(target_paths)
        assert len(generated_files) == expected_count, f"Expected {expected_count} output files, but got {len(generated_files)}"
        
        for target in target_paths:
            expected_name = f"batch_out_{target.stem}.jpg"
            expected_path = output_dir / expected_name
            assert expected_path.exists(), f"Expected output file {expected_name} not found"
            assert expected_path.stat().st_size > 0, f"Output file {expected_name} is empty"
            
        print("All output files verified successfully.")
        shutil.rmtree(test_dir)
    except Exception as e:
        print(f"Test failed or error occurred. Keeping test directory: {test_dir}")
        raise e



if __name__ == "__main__":
    try:
        test_batch_image_processing()
        print("Test PASSED")
    except AssertionError as e:
        print(f"Test FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)


if __name__ == "__main__":
    try:
        test_batch_image_processing()
        print("Test PASSED")
    except AssertionError as e:
        print(f"Test FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)
