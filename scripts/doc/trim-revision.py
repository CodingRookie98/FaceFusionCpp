#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
trim-revision.py - 修订历史滑动窗口自愈裁剪器
功能:
  1. 扫描 Markdown 文件中“修订历史记录 / Revision History”表格；
  2. 统计历史记录行数，校验是否符合 <= 5 行滑动窗口基线；
  3. 在 --fix 模式下自动就地截断并保留最近的 N 条记录（默认 5 条），移除最早的陈旧流水账；
  4. 支持 CI 门禁检查模式（超标报错 Exit Code 1，正常 Exit Code 0）。
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import List, Optional, Tuple


REVISION_HEADER_PATTERN = re.compile(
    r"^#+\s*(?:修订历史(?:记录)?|Revision\s*History)",
    re.IGNORECASE,
)


def process_markdown_file(file_path: Path, max_keep: int = 5, fix: bool = False) -> Tuple[bool, int, Optional[str]]:
    """
    处理单个 Markdown 文件的修订历史表格。
    返回: (has_overflow, current_count, new_content_or_none)
    """
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        print(f"⚠️ 无法读取文件 {file_path}: {e}", file=sys.stderr)
        return False, 0, None

    lines = content.splitlines(keepends=True)
    in_revision_section = False
    in_table = False
    table_header_lines = []
    table_data_lines = []
    table_start_idx = -1
    table_end_idx = -1

    i = 0
    while i < len(lines):
        line = lines[i]
        trimmed = line.strip()

        # 检查是否进入修订历史章节
        if REVISION_HEADER_PATTERN.search(trimmed):
            in_revision_section = True
            i += 1
            continue

        # 如果在修订历史章节中遇到下一个同级或高级标题，退出章节
        if in_revision_section and re.match(r"^#{1,3}\s+", trimmed):
            if in_table:
                # 表格已结束
                table_end_idx = i
                break
            # 遇到了其他标题
            in_revision_section = False

        if in_revision_section:
            if trimmed.startswith("|") and trimmed.endswith("|"):
                if not in_table:
                    in_table = True
                    table_start_idx = i
                    table_header_lines.append(line)
                elif len(table_header_lines) < 2:
                    # 表头分隔线行，如 | :--- | :--- |
                    table_header_lines.append(line)
                else:
                    # 数据行
                    table_data_lines.append(line)
            else:
                if in_table:
                    # 表格遇到空行或非表格行，结束表格
                    table_end_idx = i
                    break

        i += 1

    if in_table and table_end_idx == -1:
        table_end_idx = len(lines)

    if not in_table or len(table_data_lines) <= max_keep:
        return False, len(table_data_lines), None

    # 发生超额
    has_overflow = True
    overflow_count = len(table_data_lines)

    if not fix:
        return True, overflow_count, None

    # 执行就地裁剪：保留最新的 max_keep 条
    # 在本项目的修订历史中，最新记录通常追加在表格最后（或最前）。
    # 标准规范：若第 1 行是最新（降序），保留前 max_keep 行；若末行最新（升序），保留后 max_keep 行。
    # 启发式检测：比对首行和末行的版本号或日期。若无法推断，默认按本项目工程实践“后插入为最新”，保留最后 max_keep 条。
    # 如果第一行版本号高于最后一行，则首行为最新，保留前 max_keep 条。
    first_row = table_data_lines[0]
    last_row = table_data_lines[-1]
    
    first_ver_match = re.search(r"V?(\d+\.\d+(?:\.\d+)?)", first_row, re.I)
    last_ver_match = re.search(r"V?(\d+\.\d+(?:\.\d+)?)", last_row, re.I)

    # 默认为末行最新（保留最后 max_keep 条）
    kept_data_lines = table_data_lines[-max_keep:]
    if first_ver_match and last_ver_match:
        try:
            v_first = [int(x) for x in first_ver_match.group(1).split(".")]
            v_last = [int(x) for x in last_ver_match.group(1).split(".")]
            if v_first > v_last:
                # 首行版本号高于末行，说明是降序排列（最新在前），保留前 max_keep 条
                kept_data_lines = table_data_lines[:max_keep]
        except Exception:
            pass

    new_table_lines = table_header_lines + kept_data_lines
    new_lines = lines[:table_start_idx] + new_table_lines + lines[table_end_idx:]
    new_content = "".join(new_lines)
    return True, overflow_count, new_content


def scan_and_trim(root_path: Path, max_keep: int = 5, fix: bool = False) -> Tuple[int, int, List[Tuple[Path, int]]]:
    """扫描目录或单文件并执行裁剪检查"""
    md_files = []
    if root_path.is_file() and root_path.suffix == ".md":
        md_files = [root_path]
    elif root_path.is_dir():
        for f in root_path.rglob("*.md"):
            if any(p in f.parts for p in ("node_modules", ".git", ".next", "dist", "build")):
                continue
            md_files.append(f)

    total_scanned = len(md_files)
    overflow_files = []

    for file_path in sorted(md_files):
        overflow, count, new_content = process_markdown_file(file_path, max_keep=max_keep, fix=fix)
        if overflow:
            overflow_files.append((file_path, count))
            if fix and new_content is not None:
                file_path.write_text(new_content, encoding="utf-8")
                print(f"  [FIXED] {file_path.name}: 修订历史从 {count} 行成功裁剪至 {max_keep} 行")

    return total_scanned, len(overflow_files), overflow_files


def main():
    parser = argparse.ArgumentParser(description="修订历史滑动窗口自愈裁剪器")
    parser.add_argument("--root", default="docs", help="扫描根目录或单文件 (默认为 docs)")
    parser.add_argument("--keep", type=int, default=5, help="保留最近修订记录条数上限 (默认为 5)")
    parser.add_argument("--fix", action="store_true", help="自动就地裁剪超额历史记录")

    args = parser.parse_args()
    target_path = Path(args.root).resolve()

    if not target_path.exists():
        print(f"❌ 错误: 目标路径不存在: {target_path}", file=sys.stderr)
        sys.exit(1)

    mode_text = "自动裁剪并修复 (--fix)" if args.fix else "只读检查 (Check Mode)"
    print(f"🔍 启动修订历史滑动窗口审计: 目标={target_path}, 保留上限={args.keep}条, 模式={mode_text}")

    total, overflow_count, overflow_list = scan_and_trim(target_path, max_keep=args.keep, fix=args.fix)

    print(f"\n📊 审计汇总: 共检查 {total} 个文件")
    if overflow_count == 0:
        print(f"✅ 完美！所有文档的修订历史记录行数均 <= {args.keep} 条，符合滑动窗口规范！")
        sys.exit(0)
    else:
        if args.fix:
            print(f"✅ 自愈成功！已自动完成 {overflow_count} 个超标文档的修订历史裁剪。")
            sys.exit(0)
        else:
            print(f"❌ 警告: 发现 {overflow_count} 个文档的修订历史超出 {args.keep} 条上限：\n")
            for f_path, count in overflow_list:
                print(f"   📄 {f_path} (当前条数: {count} > 上限 {args.keep})")
            print(f"\n🚫 审计未通过 (Exit Code 1): 请使用 --fix 参数执行自动自愈裁剪。")
            sys.exit(1)


if __name__ == "__main__":
    main()
