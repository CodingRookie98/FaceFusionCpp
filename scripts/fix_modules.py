import os
import re

def fix_ixx_files(root_dir):
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.ixx'):
                filepath = os.path.join(root, file)
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Check if it has #include before module declaration
                has_include = re.search(r'^\s*#include', content, re.MULTILINE)
                has_global_module_fragment = re.search(r'^\s*module\s*;', content, re.MULTILINE)
                
                if has_include and not has_global_module_fragment:
                    print(f"Fixing {filepath}")
                    # Insert 'module;' at the very beginning
                    new_content = "module;\n" + content
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)

if __name__ == "__main__":
    fix_ixx_files("src")
    fix_ixx_files("tests")
