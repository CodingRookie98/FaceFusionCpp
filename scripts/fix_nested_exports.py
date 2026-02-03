import os
import re

def fix_nested_exports(root_dir):
    # Pattern to find export namespace block and capture its content
    # We use a non-greedy match for the content to handle multiple blocks if any
    pattern = re.compile(r'(export\s+namespace\s+[^{]+\s*\{)(.*?)(\}\s*//\s*namespace|\})', re.DOTALL)
    
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.ixx'):
                filepath = os.path.join(root, file)
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                new_content = content
                
                # Find all export namespace blocks
                matches = list(pattern.finditer(content))
                if not matches:
                    continue
                
                # Process from bottom to top to not mess up indices
                for match in reversed(matches):
                    prefix = match.group(1)
                    inner_content = match.group(2)
                    suffix = match.group(3)
                    
                    # Remove 'export ' from lines inside the block
                    # Only if it's at the start of a line or after some whitespace
                    new_inner = re.sub(r'(^|\n)(\s*)export\s+', r'\1\2', inner_content)
                    
                    if new_inner != inner_content:
                        print(f"Fixing nested exports in {filepath}")
                        new_content = new_content[:match.start()] + prefix + new_inner + suffix + new_content[match.end():]
                
                if new_content != content:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(new_content)

if __name__ == "__main__":
    fix_nested_exports("src")
    fix_nested_exports("tests")
