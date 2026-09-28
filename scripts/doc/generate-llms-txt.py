#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
generate-llms-txt.py - 机器可读 llms.txt 自动化生成器
功能:
  1. 扫描 docs/ 目录下的所有 Markdown 文档；
  2. 提取每篇文档的标题、Diátaxis 象限分类与精炼摘要；
  3. 按照 llmstxt.org 标准规范生成紧凑、结构化的 docs/llms.txt；
  4. 赋予 AI 智能体在毫秒级掌握全域知识拓扑与权威入口的能力。
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple


def extract_doc_info(file_path: Path) -> Tuple[str, str]:
    """提取文档的标题与摘要描述"""
    try:
        content = file_path.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return file_path.stem, ""

    lines = content.splitlines()
    title = file_path.stem
    description = ""

    # 1. 查找第一个 Markdown 一级标题
    for line in lines:
        m = re.match(r"^#\s+(.+)$", line.strip())
        if m:
            title = m.group(1).strip()
            # 移除标题中的 Markdown 格式
            title = re.sub(r"[*_`]", "", title)
            break

    # 2. 查找元数据描述或首段非标题文本
    # 模式 A: 检查 Frontmatter 中的 description / 描述 / 修订描述
    desc_match = re.search(r"(?:description|描述|修订描述|简述)\s*:\s*([^\n]+)", content, re.I)
    if desc_match:
        description = desc_match.group(1).strip()
    else:
        # 模式 B: 查找第一个有意义的正文段落
        in_code = False
        for line in lines:
            trimmed = line.strip()
            if trimmed.startswith("```"):
                in_code = not in_code
                continue
            if in_code or not trimmed:
                continue
            if trimmed.startswith("#") or trimmed.startswith(">") or trimmed.startswith("|") or trimmed.startswith("-"):
                continue
            # 找到了普通正文行
            description = trimmed[:120] + "..." if len(trimmed) > 120 else trimmed
            break

    return title, description


SECTION_ORDER = [
    ("proposals", "Proposals & RFCs (需求与设计孵化层)"),
    ("tutorials", "Tutorials (学习导向 / 新手上手教程)"),
    ("how-to", "How-To Guides (操作指南 / 问题与任务 SOP)"),
    ("reference", "Reference (技术参考 / 机器事实唯一真相源)"),
    ("explanation", "Explanation (深度剖析 / 系统架构与 ADR 决策)"),
    ("project", "Project Governance (工程演进 / 发布日志与待办)"),
]


def generate_llms_txt(root_dir: Path, output_file: Path, project_name: str = "Project"):
    """生成 llms.txt"""
    sections: Dict[str, List[Tuple[str, str, str]]] = {k: [] for k, _ in SECTION_ORDER}
    other_docs: List[Tuple[str, str, str]] = []

    all_files = sorted(list(root_dir.rglob("*.md")))

    for f in all_files:
        if any(p in f.parts for p in ("node_modules", ".git", ".next", "dist", "build")):
            continue
        rel_parts = f.relative_to(root_dir).parts
        if len(rel_parts) == 1:
            # 根文件（如 index.md, GOVERNANCE.md）
            continue

        top_dir = rel_parts[0]
        title, desc = extract_doc_info(f)
        rel_link = f.relative_to(root_dir).as_posix()

        if top_dir in sections:
            sections[top_dir].append((title, rel_link, desc))
        else:
            other_docs.append((title, rel_link, desc))

    output_lines = [
        f"# {project_name} Machine-Readable Knowledge Base Map",
        "",
        f"> 本文件遵循 llmstxt.org 规范，为 AI 智能体提供全景知识拓扑与物理文件导航地图。",
        f"> 严禁凭空猜测路径，查阅具体模块前请通过下述相对链接精准索引对应文件。",
        "",
    ]

    for sec_key, sec_title in SECTION_ORDER:
        docs = sections.get(sec_key, [])
        if not docs:
            continue
        output_lines.append(f"## {sec_title}")
        output_lines.append("")
        for title, link, desc in docs:
            desc_text = f": {desc}" if desc else ""
            output_lines.append(f"- [{title}]({link}){desc_text}")
        output_lines.append("")

    if other_docs:
        output_lines.append("## Other Documentation (其他文档)")
        output_lines.append("")
        for title, link, desc in other_docs:
            desc_text = f": {desc}" if desc else ""
            output_lines.append(f"- [{title}]({link}){desc_text}")
        output_lines.append("")

    content = "\n".join(output_lines)
    output_file.write_text(content, encoding="utf-8")
    print(f"✅ 机器可读地图成功生成至: {output_file} (共收录 {sum(len(v) for v in sections.values()) + len(other_docs)} 篇有效文档)")


def main():
    parser = argparse.ArgumentParser(description="机器可读 llms.txt 自动化生成器")
    parser.add_argument("--root", default="docs", help="文档根目录 (默认 docs)")
    parser.add_argument("--output", default="docs/llms.txt", help="输出路径 (默认 docs/llms.txt)")
    parser.add_argument("--name", default="System", help="项目名称")

    args = parser.parse_args()
    root_path = Path(args.root).resolve()
    out_path = Path(args.output).resolve()

    if not root_path.exists():
        print(f"❌ 错误: 目标根目录不存在: {root_path}", file=sys.stderr)
        sys.exit(1)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    generate_llms_txt(root_path, out_path, project_name=args.name)


if __name__ == "__main__":
    main()
