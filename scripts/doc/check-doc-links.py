#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check-doc-links.py - 全域物理文件链接与锚点断链静态扫描器
功能:
  1. 递归扫描指定根目录下的所有 Markdown 文件；
  2. 提取所有相对超链接，校验目标文件物理是否存在；
  3. 检查指向 Markdown 文件的相对链接是否严格保留 .md 物理后缀；
  4. 若携带 #anchor 锚点，校验目标文件中是否存在对应的标题 Slug；
  5. 发现任何断链或格式违规时输出详细位置并以 Exit Code 1 阻断，全部通过返回 Exit Code 0。
"""

import argparse
import os
import re
import sys
import urllib.parse
from pathlib import Path
from typing import Dict, List, Set, Tuple


def slugify_heading(heading: str) -> str:
    """将 Markdown 标题转换为 GitHub 规范的锚点 Slug"""
    # 移除行首的 # 符号与空白
    text = re.sub(r"^#+\s*", "", heading).strip()
    # 移除行内的 Markdown 格式标记（链接、加粗、斜体、代码行）
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    # 注意：下划线 '_' 在 GitHub 锚点中保留，不可作为格式标记移除
    text = re.sub(r"[*`~]", "", text)
    # 转为小写
    text = text.lower()
    # 移除除字母、数字、中文、空格、下划线、短横线之外的字符
    text = re.sub(r"[^\w\s\-_\u4e00-\u9fff]", "", text)
    # GitHub 规则：每个空白字符各替换为一个短横线（连续空格产生连续短横线）
    slug = re.sub(r"\s", "-", text)
    return slug


def extract_headings(file_path: Path) -> Set[str]:
    """提取文件中的所有标题 Slug"""
    slugs = set()
    slug_counts: Dict[str, int] = {}
    if not file_path.is_file():
        return slugs

    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return slugs

    in_code_block = False
    for line in content.splitlines():
        trimmed = line.strip()
        if trimmed.startswith("```"):
            in_code_block = not in_code_block
            continue
        if in_code_block:
            continue

        if trimmed.startswith("#") and re.match(r"^#{1,6}\s+", trimmed):
            raw_slug = slugify_heading(trimmed)
            if not raw_slug:
                continue
            # GitHub 处理重复标题锚点机制：-1, -2 ...
            if raw_slug in slug_counts:
                count = slug_counts[raw_slug]
                slug_counts[raw_slug] += 1
                unique_slug = f"{raw_slug}-{count}"
            else:
                slug_counts[raw_slug] = 1
                unique_slug = raw_slug
            slugs.add(unique_slug)
            slugs.add(raw_slug)  # 同时支持基础 slug 匹配
    return slugs


# 匹配 Markdown 链接：[text](target) 或 ![text](target)
LINK_PATTERN = re.compile(r"!?\[([^\]]*)\]\(([^)]+)\)")


def check_file_links(
    file_path: Path,
    root_dir: Path,
    headings_cache: Dict[Path, Set[str]],
    strict_md_extension: bool = True,
) -> List[Tuple[int, str, str, str]]:
    """
    检查单个文件中的所有链接。
    返回错误列表: [(line_no, link_text, raw_target, error_reason)]
    """
    errors = []
    try:
        lines = file_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except Exception as e:
        return [(0, "", "", f"无法读取文件: {e}")]

    in_code_block = False
    for line_idx, line in enumerate(lines, start=1):
        trimmed = line.strip()
        if trimmed.startswith("```"):
            in_code_block = not in_code_block
            continue
        if in_code_block:
            continue

        # 预先剥离被反引号整体包裹的行内代码示例: `[text](target)`
        sanitized_line = re.sub(r"`\[[^`]*\]\([^`]*\)`", "", line)

        for match in LINK_PATTERN.finditer(sanitized_line):
            link_text = match.group(1).strip()
            raw_target = match.group(2).strip()

            # 拆分可能存在的 title 描述，如: (path/to/file "title")
            if " " in raw_target and not raw_target.startswith("http"):
                parts = raw_target.split(None, 1)
                raw_target = parts[0]

            # 忽略外部链接与协议
            if re.match(r"^(https?://|mailto:|ftp:|javascript:|conversation://)", raw_target, re.I):
                continue

            # 忽略未实例化的模板占位符链接（如 {name} / {title} / {plan_name} / {task_name}）
            if "{" in raw_target or "}" in raw_target:
                continue

            # 忽略 HTML 页面内专用 JS 伪链接
            if raw_target.startswith("#"):
                # 本文件内的锚点检查
                anchor = raw_target[1:].strip().lower()
                if not anchor:
                    continue
                if file_path not in headings_cache:
                    headings_cache[file_path] = extract_headings(file_path)
                if anchor not in headings_cache[file_path]:
                    errors.append((line_idx, link_text, raw_target, f"当前文件内未找到锚点 '#{anchor}'"))
                continue

            # 处理路径与锚点
            if "#" in raw_target:
                path_part, anchor_part = raw_target.split("#", 1)
                anchor = anchor_part.strip().lower()
            else:
                path_part, anchor = raw_target, ""

            # URL 解码
            decoded_path = urllib.parse.unquote(path_part)

            # 解析目标物理路径
            if decoded_path.startswith("/"):
                # 根路径相对
                target_file = (root_dir / decoded_path.lstrip("/")).resolve()
            else:
                target_file = (file_path.parent / decoded_path).resolve()

            # 检查目标文件物理是否存在
            if not target_file.exists():
                errors.append((line_idx, link_text, raw_target, f"目标路径不存在 (404 Not Found): {target_file}"))
                continue

            # 如果目标是目录，检查是否存在 index.md 或 README.md
            if target_file.is_dir():
                has_index = (target_file / "index.md").exists() or (target_file / "README.md").exists()
                if not has_index:
                    errors.append((line_idx, link_text, raw_target, f"链接指向目录但缺失 index.md/README.md: {target_file}"))
                continue

            # 严格模式：如果目标是 Markdown 文件，要求必须带有 .md 后缀
            if strict_md_extension and target_file.is_file():
                if target_file.suffix == ".md" and not path_part.lower().endswith(".md"):
                    errors.append((line_idx, link_text, raw_target, "违背显式文件链接规范：指向 Markdown 文件的超链接缺失 '.md' 后缀"))

            # 校验锚点有效性
            if anchor and target_file.suffix == ".md":
                if target_file not in headings_cache:
                    headings_cache[target_file] = extract_headings(target_file)
                if anchor not in headings_cache[target_file]:
                    errors.append((line_idx, link_text, raw_target, f"目标文件存在但未找到对应锚点 '#{anchor}'"))

    return errors


def scan_directory(
    root_dir: Path,
    target_dirs: List[str],
    strict_md: bool = True,
    verbose: bool = False,
) -> Tuple[int, int, Dict[Path, List[Tuple[int, str, str, str]]]]:
    """扫描指定目录下的 Markdown 链接"""
    headings_cache: Dict[Path, Set[str]] = {}
    total_files = 0
    total_errors = 0
    file_errors: Dict[Path, List[Tuple[int, str, str, str]]] = {}

    search_roots = [root_dir / d for d in target_dirs] if target_dirs else [root_dir]

    md_files = []
    for s_root in search_roots:
        if not s_root.exists():
            continue
        if s_root.is_file() and s_root.suffix == ".md":
            md_files.append(s_root)
        elif s_root.is_dir():
            md_files.extend(list(s_root.rglob("*.md")))

    # 去重并排序
    md_files = sorted(list(set(md_files)))

    for md_file in md_files:
        # 跳过 node_modules, .git, .agents 等内部目录
        parts = md_file.parts
        if any(p in parts for p in ("node_modules", ".git", ".next", "dist", "build")):
            continue

        total_files += 1
        errs = check_file_links(md_file, root_dir, headings_cache, strict_md_extension=strict_md)
        if errs:
            file_errors[md_file] = errs
            total_errors += len(errs)
        elif verbose:
            print(f"  [PASS] {md_file.relative_to(root_dir)}")

    return total_files, total_errors, file_errors


def main():
    parser = argparse.ArgumentParser(description="全域 Markdown 物理链接与锚点断链静态扫描器")
    parser.add_argument("--root", default="docs", help="扫描根目录 (默认为 docs)")
    parser.add_argument("--dir", action="append", help="指定扫描子目录或文件 (可多次指定)")
    parser.add_argument("--no-strict-ext", action="store_true", help="允许链接指向 Markdown 时省略 .md 后缀 (不推荐)")
    parser.add_argument("--verbose", "-v", action="store_true", help="输出通过的正常文件")

    args = parser.parse_args()
    root_path = Path(args.root).resolve()

    if not root_path.exists():
        print(f"❌ 错误: 目标根目录不存在: {root_path}", file=sys.stderr)
        sys.exit(1)

    print(f"🔍 启动物理链接与锚点静态审计: 根目录={root_path} ...")
    total_files, total_errors, file_errors = scan_directory(
        root_dir=root_path,
        target_dirs=args.dir,
        strict_md=not args.no_strict_ext,
        verbose=args.verbose,
    )

    print(f"\n📊 审计汇总: 共扫描 {total_files} 个 Markdown 文件")
    if total_errors == 0:
        print("✅ 恭喜！未发现任何 404 断链或非法相对引用，所有链接 100% 物理有效！")
        sys.exit(0)
    else:
        print(f"❌ 警告: 发现 {total_errors} 处无效链接或断链违规：\n")
        for f_path, errs in file_errors.items():
            try:
                rel_path = f_path.relative_to(root_path)
            except ValueError:
                rel_path = f_path
            print(f"📄 【{rel_path}】:")
            for line_no, text, target, reason in errs:
                print(f"   第 {line_no} 行 | 链接文本: '{text}' ➔ 目标: '{target}'")
                print(f"   ↳ 错误原因: {reason}")
            print()
        print(f"🚫 审计未通过 (Exit Code 1): 请就地修复上述 {total_errors} 处断链后重新运行。")
        sys.exit(1)


if __name__ == "__main__":
    main()
