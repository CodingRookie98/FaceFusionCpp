import os
import sys
import re

def patch_opencv_types_hpp(vcpkg_root):
    """
    Patches OpenCV's types.hpp to resolve C++20 ambiguity in Rect class.
    """
    # Common paths for types.hpp in vcpkg
    possible_paths = [
        os.path.join(vcpkg_root, "x64-linux/include/opencv4/opencv2/core/types.hpp"),
        os.path.join(vcpkg_root, "x64-windows/include/opencv2/core/types.hpp"),
        os.path.join(vcpkg_root, "include/opencv2/core/types.hpp"),
    ]
    
    target_file = None
    for path in possible_paths:
        if os.path.exists(path):
            target_file = path
            break
            
    if not target_file:
        print(f"Could not find OpenCV types.hpp in {vcpkg_root}")
        return False

    with open(target_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # The problematic line is usually 'a = Rect();' inside 'Rect_& operator = (const Rect_& r)'
    # We want to change it to 'a = Rect_<_Tp>();' to be explicit and avoid C++20 ambiguity
    
    pattern = r'(\s+)a\s*=\s*Rect\(\);'
    replacement = r'\1a = Rect_<_Tp>();'
    
    if 'a = Rect_<_Tp>();' in content:
        print(f"OpenCV patch already applied to {target_file}")
        return True
        
    new_content = re.sub(pattern, replacement, content)
    
    if new_content == content:
        print(f"Could not find the pattern to patch in {target_file}")
        return False
        
    with open(target_file, 'w', encoding='utf-8') as f:
        f.write(new_content)
        
    print(f"Successfully patched {target_file}")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python patch_opencv.py <vcpkg_installed_path>")
        sys.exit(1)
        
    vcpkg_installed = sys.argv[1]
    if patch_opencv_types_hpp(vcpkg_installed):
        sys.exit(0)
    else:
        # We don't necessarily want to fail the build if patching fails, 
        # as it might already be fixed in newer OpenCV versions.
        sys.exit(0)
