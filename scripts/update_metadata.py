#!/usr/bin/env python3
import yaml
import re
import os
import sys
import argparse

def load_metadata(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        return yaml.safe_load(f)

def update_file(file_path, metadata, marker_pattern, replacement_map, dry_run=False):
    if not os.path.exists(file_path):
        print(f"Warning: File {file_path} not found. Skipping.")
        return

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    new_content = content
    for pattern, value in replacement_map.items():
        # Handle both literal markers and regex patterns
        new_content = re.sub(pattern, value, new_content)

    if new_content != content:
        if dry_run:
            print(f"Changes detected for {file_path} (dry-run)")
            # In a real tool, we might show a diff here
        else:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Updated {file_path}")
    else:
        print(f"No changes needed for {file_path}")

def main():
    parser = argparse.ArgumentParser(description="Update project metadata in source files.")
    parser.add_argument("--dry-run", action="store_true", help="Show changes without applying them.")
    args = parser.parse_args()

    # Get root directory relative to this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)
    metadata_path = os.path.join(root_dir, "project.yaml")

    if not os.path.exists(metadata_path):
        print(f"Error: project.yaml not found at {metadata_path}")
        sys.exit(1)

    metadata = load_metadata(metadata_path)
    proj = metadata['project']

    # 1. Update config_types.ixx
    update_file(
        os.path.join(root_dir, "src/app/config/config_types.ixx"),
        metadata,
        None,
        {
            r'inline constexpr const char\* kSupportedConfigVersion = ".*?";':
            f'inline constexpr const char* kSupportedConfigVersion = "{proj["version"]}";',
            r'\* @date \d{4}-\d{2}-\d{2}': f'* @date {proj["date"]}',
            r'\* @author \w+': f'* @author {proj["author"]}'
        },
        args.dry_run
    )

    # 2. Update CMakeLists.txt
    update_file(
        os.path.join(root_dir, "CMakeLists.txt"),
        metadata,
        None,
        {
            r'project\(\w+': f'project({proj["name"]}',
            r'VERSION \d+\.\d+\.\d+': f'VERSION {proj["version"]}',
            r'set\(CPACK_PACKAGE_NAME \$\{PROJECT_NAME\}\)': f'set(CPACK_PACKAGE_NAME "{proj["name"]}")'
        },
        args.dry_run
    )

    # 3. Update vcpkg.json
    update_file(
        os.path.join(root_dir, "vcpkg.json"),
        metadata,
        None,
        {
            r'"version": ".*?"': f'"version": "{proj["version"]}"',
            r'"description": ".*?"': f'"description": "{proj["description"]}"',
            r'"license": ".*?"': f'"license": "{proj["license"]}"'
        },
        args.dry_run
    )

    # 4. Update README.md (Header info)
    update_file(
        os.path.join(root_dir, "README.md"),
        metadata,
        None,
        {
            r'# .*': f'# {proj["name"]}',
            r'> \*\*.*\*\*': f'> **{proj["description"]}**'
        },
        args.dry_run
    )

if __name__ == "__main__":
    main()
