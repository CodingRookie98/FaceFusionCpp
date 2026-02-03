import os
import re

def audit_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check for 'export namespace'
    has_export_namespace = re.search(r'export\s+namespace\s+[\w:]+\s*\{', content)
    
    if has_export_namespace:
        # Find 'export' keywords inside the namespace block
        # This is a bit tricky with regex, but we can look for 'export' followed by 
        # class, struct, enum, void, etc. that are NOT 'export namespace' or 'export module'
        
        # Pattern to find 'export' that is likely redundant
        # It looks for 'export' followed by common declaration keywords
        redundant_pattern = r'export\s+(class|struct|enum|void|std|template|auto|inline|static)'
        
        matches = list(re.finditer(redundant_pattern, content))
        if matches:
            print(f"Found {len(matches)} redundant exports in {filepath}")
            # Fix them: remove 'export ' before these keywords
            new_content = re.sub(redundant_pattern, r'\1', content)
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            return True
    return False

def main():
    ixx_files = []
    for root, dirs, files in os.walk('src'):
        for file in files:
            if file.endswith('.ixx'):
                ixx_files.append(os.path.join(root, file))
    
    fixed_count = 0
    for file in ixx_files:
        if audit_file(file):
            fixed_count += 1
            
    print(f"Audited {len(ixx_files)} files, fixed {fixed_count} files.")

if __name__ == "__main__":
    main()
