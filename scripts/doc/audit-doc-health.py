#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
audit-doc-health.py - 知识库全面健康度体检引擎
功能:
  1. 扫描整个文档目录，统计 Diátaxis 四象限与工程演进分布；
  2. 审计 Frontmatter 元数据控制头（版本号、标识、所有者、状态）合规率；
  3. 审计修订历史滑动窗口合规率（<= 5 条）；
  4. 审计全域物理断链与 404 错误；
  5. 探测孤儿文档（未被 index.md 或其他文档引用的孤立文件）；
  6. 支持 --compat 模式：探测既有瀑布老目录（requirements, design 等）并出具平滑迁移映射建议；
  7. 综合计算健康评分 (0-100 分)，出具结构化诊断报告。
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

import importlib.util

# 动态加载同目录下带有短横线名称的兄弟脚本
_scripts_dir = Path(__file__).resolve().parent

def _load_sibling(filename: str, module_name: str):
    script_path = _scripts_dir / filename
    spec = importlib.util.spec_from_file_location(module_name, script_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"无法加载兄弟模块: {script_path}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

_links_mod = _load_sibling("check-doc-links.py", "check_doc_links")
_trim_mod = _load_sibling("trim-revision.py", "trim_revision")

scan_directory = _links_mod.scan_directory
process_markdown_file = _trim_mod.process_markdown_file


DIATAXIS_CATEGORIES = {
    "tutorials": "🎓 教程象限 (Tutorials)",
    "how-to": "🛠️ 操作指南象限 (How-To Guides)",
    "reference": "📖 技术参考象限 (Reference)",
    "explanation": "💡 深度剖析象限 (Explanation)",
    "proposals": "💡 需求设计孵化层 (Proposals / RFC)",
    "project": "🚀 工程演进与管理 (Project Governance)",
}

LEGACY_CATEGORIES = {
    "requirements": "⚠️ 历史需求目录 (建议迁移至 reference/ 或 proposals/)",
    "design": "⚠️ 历史设计目录 (建议迁移至 explanation/architecture/)",
    "planning": "⚠️ 历史规划目录 (建议迁移至 explanation/analysis/ 或 project/)",
    "operations": "⚠️ 历史运维目录 (建议迁移至 how-to/ 或 tutorials/)",
}


def parse_frontmatter(content: str) -> Dict[str, str]:
    """解析文档头部的 Frontmatter 或文档控制信息块"""
    metadata = {}
    # 模式 A: YAML Frontmatter (--- ... ---)
    yaml_match = re.match(r"^---\s*\n(.*?)\n---\s*\n", content, re.DOTALL)
    if yaml_match:
        for line in yaml_match.group(1).splitlines():
            if ":" in line:
                k, v = line.split(":", 1)
                metadata[k.strip().lower()] = v.strip()
        return metadata

    # 模式 B: 引用块元数据 (支持 > **文档控制信息** 或直接 > **文档标识** / > **文档版本**)
    for line in content.splitlines()[:50]:
        trimmed = line.strip()
        if trimmed.startswith(">"):
            clean = re.sub(r"^[>\s\-*]+", "", trimmed).strip()
            if ":" in clean or "：" in clean:
                sep = ":" if ":" in clean else "："
                k, v = clean.split(sep, 1)
                k_clean = re.sub(r"[*_`]", "", k).strip().lower()
                metadata[k_clean] = v.strip()

    # 模式 C: 表格型元数据 (| **文档标识** | DR-xxx | 或 | 文档标识 | ... |)
    if "文档标识" in content or "当前版本" in content or "version" in content.lower() or "文档版本" in content:
        for m in re.finditer(r"\|\s*(?:\*\*)?([^|\n*]+?)(?:\*\*)?\s*\|\s*([^|\n]+?)\s*\|", content):
            k_raw = m.group(1).strip().lower()
            v_raw = m.group(2).strip()
            if k_raw in ("文档标识", "当前版本", "规范版本", "文档版本", "version", "id", "文档所有者", "生效日期"):
                metadata[k_raw] = v_raw

    return metadata


def audit_health(root_dir: Path, compat_mode: bool = False) -> Dict:
    """执行全面的健康度体检"""
    all_md_files = [
        f for f in root_dir.rglob("*.md")
        if not any(p in f.parts for p in ("node_modules", ".git", ".next", "dist", "build"))
    ]

    total_files = len(all_md_files)
    if total_files == 0:
        return {"error": f"未在 {root_dir} 下找到任何 Markdown 文档"}

    # 1. 拓扑分类统计
    category_counts: Dict[str, List[Path]] = {cat: [] for cat in DIATAXIS_CATEGORIES}
    legacy_counts: Dict[str, List[Path]] = {cat: [] for cat in LEGACY_CATEGORIES}
    root_level_docs = []

    for f in all_md_files:
        rel_parts = f.relative_to(root_dir).parts
        if len(rel_parts) == 1:
            root_level_docs.append(f)
            continue
        top_dir = rel_parts[0]
        if top_dir in DIATAXIS_CATEGORIES:
            category_counts[top_dir].append(f)
        elif top_dir in LEGACY_CATEGORIES:
            legacy_counts[top_dir].append(f)
        else:
            category_counts.setdefault("other", []).append(f)

    # 2. 控制信息基线检查
    valid_meta_files = 0
    missing_meta_files = []

    # 3. 修订历史检查
    overflow_rev_files = []

    # 4. 收集出链目标，计算引用图与孤儿文档
    referenced_files: Set[Path] = set()
    link_pattern = re.compile(r"!?\[([^\]]*)\]\(([^)]+)\)")

    for f in all_md_files:
        content = f.read_text(encoding="utf-8", errors="replace")
        meta = parse_frontmatter(content)
        # 判定控制元数据是否合规：至少包含 version 或 当前版本
        # 项目控制头键名含英文括注（如 "文档标识 (Document ID)" / "当前版本 (Version)"），故按子串匹配
        meta_keys = " ".join(meta.keys())
        has_version = any(k in meta_keys for k in ("version", "当前版本", "规范版本", "文档版本", "版本"))
        has_id = any(k in meta_keys for k in ("id", "文档标识", "标识", "name", "doc_id"))
        is_archived = "archived" in f.parts
        if (has_version and has_id) or (compat_mode and is_archived):
            valid_meta_files += 1
        else:
            missing_meta_files.append(f)

        # 修订历史检查
        overflow, count, _ = process_markdown_file(f, max_keep=5, fix=False)
        if overflow:
            overflow_rev_files.append((f, count))

        # 解析链接目标
        for m in link_pattern.finditer(content):
            target = m.group(2).strip().split("#")[0].split(" ")[0]
            if not target or re.match(r"^(https?://|mailto:|javascript:|ftp:)", target, re.I):
                continue
            try:
                resolved = (f.parent / target).resolve()
                if resolved.exists() and resolved.is_file() and resolved.suffix == ".md":
                    referenced_files.add(resolved)
            except Exception:
                pass

    # 孤儿文档判定 (不包含根目录下的 index.md, llms.txt, GOVERNANCE.md 等入口)
    orphan_docs = []
    for f in all_md_files:
        if f.name in ("index.md", "README.md", "GOVERNANCE.md", "llms.txt"):
            continue
        if f not in referenced_files:
            orphan_docs.append(f)

    # 5. 断链扫描
    _, total_link_errors, file_link_errors = scan_directory(
        root_dir=root_dir,
        target_dirs=[],
        strict_md=True,
        verbose=False,
    )

    # 计算各项得分 (满分 100)
    # 权重: 链接完整率 (35分), 元数据基线率 (25分), 修订历史合规率 (20分), 孤儿文档率 (10分), 架构规范度 (10分)
    link_score = 35 if total_link_errors == 0 else max(0, 35 - total_link_errors * 5)
    meta_score = (valid_meta_files / total_files) * 25
    rev_score = ((total_files - len(overflow_rev_files)) / total_files) * 20
    orphan_score = ((total_files - len(orphan_docs)) / total_files) * 10
    
    # 架构规范度：在标准模式下若存在大量 legacy 目录扣分，compat 模式下给予豁免
    legacy_count = sum(len(v) for v in legacy_counts.values())
    if compat_mode:
        arch_score = 10
    else:
        arch_score = 10 if legacy_count == 0 else max(0, 10 - legacy_count * 2)

    total_score = round(link_score + meta_score + rev_score + orphan_score + arch_score, 1)

    return {
        "total_files": total_files,
        "total_score": total_score,
        "category_counts": category_counts,
        "legacy_counts": legacy_counts,
        "root_docs": root_level_docs,
        "valid_meta_files": valid_meta_files,
        "missing_meta_files": missing_meta_files,
        "overflow_rev_files": overflow_rev_files,
        "total_link_errors": total_link_errors,
        "file_link_errors": file_link_errors,
        "orphan_docs": orphan_docs,
        "scores": {
            "link_score": round(link_score, 1),
            "meta_score": round(meta_score, 1),
            "rev_score": round(rev_score, 1),
            "orphan_score": round(orphan_score, 1),
            "arch_score": round(arch_score, 1),
        },
    }


def main():
    parser = argparse.ArgumentParser(description="知识库全面健康度体检引擎")
    parser.add_argument("--root", default="docs", help="文档根目录 (默认为 docs)")
    parser.add_argument("--compat", action="store_true", help="启用既有项目老目录兼容模式")
    parser.add_argument("--threshold", type=float, default=80.0, help="合格通过分数门槛 (默认 80.0)")

    args = parser.parse_args()
    root_path = Path(args.root).resolve()

    if not root_path.exists():
        print(f"❌ 错误: 目标文档目录不存在: {root_path}", file=sys.stderr)
        sys.exit(1)

    print(f"🏥 启动知识库全面健康度体检: 根目录={root_path} ...")
    res = audit_health(root_path, compat_mode=args.compat)
    if "error" in res:
        print(f"❌ {res['error']}", file=sys.stderr)
        sys.exit(1)

    print("=" * 60)
    print(f"📊 知识库健康体检综合诊断报告")
    print(f"   总得分: {res['total_score']} / 100 分  (通过门槛: {args.threshold}分)")
    print("=" * 60)

    # 1. 维度细分评分
    scores = res["scores"]
    print("\n📈 [各项细分评分]:")
    print(f"   * 物理链接与断链防护: {scores['link_score']} / 35 分 (断链数: {res['total_link_errors']})")
    print(f"   * 控制信息元数据基线: {scores['meta_score']} / 25 分 (合规文档: {res['valid_meta_files']}/{res['total_files']})")
    print(f"   * 修订历史滑动窗口:   {scores['rev_score']} / 20 分 (超标文档: {len(res['overflow_rev_files'])}/{res['total_files']})")
    print(f"   * 孤儿文档引用度:     {scores['orphan_score']} / 10 分 (孤立文档: {len(res['orphan_docs'])}/{res['total_files']})")
    print(f"   * Diátaxis 架构规范:  {scores['arch_score']} / 10 分")

    # 2. 目录拓扑分布
    print("\n📂 [知识库拓扑分布]:")
    for cat, name in DIATAXIS_CATEGORIES.items():
        docs = res["category_counts"].get(cat, [])
        print(f"   * {name}: {len(docs)} 篇")
    
    legacy_total = sum(len(v) for v in res["legacy_counts"].values())
    if legacy_total > 0:
        print(f"\n⚠️  [探测到历史瀑布结构文档 ({legacy_total} 篇)]:")
        for cat, name in LEGACY_CATEGORIES.items():
            docs = res["legacy_counts"].get(cat, [])
            if docs:
                print(f"   * {name}: {len(docs)} 篇")

    # 3. 诊断发现与整改建议
    print("\n💡 [体检诊断与修复建议]:")
    has_issues = False

    if res["total_link_errors"] > 0:
        has_issues = True
        print(f"   ❌ 存在 {res['total_link_errors']} 处断链，运行 `python3 check-doc-links.py --root {args.root}` 查看详情并修复。")

    if res["overflow_rev_files"]:
        has_issues = True
        print(f"   ⚠️  存在 {len(res['overflow_rev_files'])} 篇文档修订历史超过 5 条，运行 `python3 trim-revision.py --root {args.root} --fix` 自动自愈。")

    if res["missing_meta_files"]:
        has_issues = True
        sample = [f.name for f in res["missing_meta_files"][:3]]
        print(f"   ⚠️  存在 {len(res['missing_meta_files'])} 篇文档缺少标准控制头 (示例: {', '.join(sample)})。")

    if res["orphan_docs"]:
        has_issues = True
        sample = [f.name for f in res["orphan_docs"][:3]]
        print(f"   ⚠️  存在 {len(res['orphan_docs'])} 篇孤儿文档未被索引引用 (示例: {', '.join(sample)})，建议在 index.md 中收录。")

    if not has_issues:
        print("   🌟 完美！未检测到任何健康缺陷，知识库处于极佳健康状态！")

    print("\n" + "=" * 60)
    if res["total_score"] >= args.threshold and res["total_link_errors"] == 0:
        print("🏁 诊断结论: PASS (健康度达标，准予交付) ✅")
        sys.exit(0)
    else:
        print(f"🚫 诊断结论: REJECTED (得分 {res['total_score']} < 门槛 {args.threshold} 或存在断链，请修复后重测)")
        sys.exit(1)


if __name__ == "__main__":
    main()
