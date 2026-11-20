#!/usr/bin/env bash
# ==============================================================================
# scaffold-doc.sh - Diátaxis & RFC 标准文档脚手架自动化生成器
# 用法:
#   scaffold-doc.sh <type> <name> [--root docs] [--lang zh|en]
# 类型 (<type>):
#   tutorial    - 新手上门教程 (docs/tutorials/)
#   how-to      - 任务操作指南 (docs/how-to/)
#   reference   - 权威技术参考 (docs/reference/)
#   explanation - 深度架构剖析 (docs/explanation/)
#   adr         - 架构决策记录 (docs/explanation/decisions/000X-name.md)
#   rfc         - 需求与设计提案 (docs/proposals/RFC-000X-name.md)
# ==============================================================================

set -euo pipefail

TYPE="${1:-}"
NAME="${2:-}"
ROOT_DIR="docs"
LANG="zh"

if [ -z "$TYPE" ] || [ -z "$NAME" ]; then
  echo "❌ 错误: 必须指定文档类型和文档名称。"
  echo "用法: $0 <tutorial|how-to|reference|explanation|adr|rfc> <doc-name> [--root docs] [--lang zh|en]"
  exit 1
fi

# 参数解析
shift 2 || true
while [ $# -gt 0 ]; do
  case "$1" in
    --root)
      ROOT_DIR="$2"
      shift 2
      ;;
    --lang)
      LANG="$2"
      shift 2
      ;;
    *)
      echo "未知参数: $1"
      exit 1
      ;;
  esac
done

TODAY=$(date +%Y-%m-%d)
YEAR=$(date +%Y)

# 确保根目录存在
mkdir -p "$ROOT_DIR"

case "$TYPE" in
  tutorial)
    TARGET_DIR="$ROOT_DIR/tutorials"
    mkdir -p "$TARGET_DIR"
    FILE_PATH="$TARGET_DIR/${NAME}.md"
    cat <<EOF > "$FILE_PATH"
# ${NAME} 新手上路指南 (Tutorial)

> **文档控制信息**
> - **文档标识**: TUT-${NAME^^}-${YEAR}
> - **当前版本**: V1.0.0
> - **文档状态**: Active
> - **生效日期**: ${TODAY}
> - **文档所有者**: AI Agent / 工程师

---

### 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | ${TODAY} | AI Agent | 架构师 | 初始化新手上路教程脚手架 |

---

## 1. 学习目标 (What You'll Learn)
完成本教程后，你将能够：
1. 搭建最小运行环境；
2. 运行首个核心业务链路；
3. 验证端到端交付结果。

## 2. 前置准备 (Prerequisites)
- 环境依赖说明（如 Node.js >= 20, Python >= 3.10）

## 3. 循序渐进操作步骤 (Step-by-Step)
### 步骤 1: 准备环境
\`\`\`bash
# 示例命令
\`\`\`

### 步骤 2: 启动最小示例
\`\`\`bash
# 示例命令
\`\`\`

## 4. 下一步探索 (Next Steps)
- 探索更多实战操作: [how-to 指南](../how-to/index.md)
EOF
    ;;

  how-to)
    TARGET_DIR="$ROOT_DIR/how-to"
    mkdir -p "$TARGET_DIR"
    FILE_PATH="$TARGET_DIR/${NAME}.md"
    cat <<EOF > "$FILE_PATH"
# ${NAME} 操作指南 (How-To Guide)

> **文档控制信息**
> - **文档标识**: HOWTO-${NAME^^}-${YEAR}
> - **当前版本**: V1.0.0
> - **文档状态**: Active
> - **生效日期**: ${TODAY}
> - **文档所有者**: AI Agent / 工程师

---

### 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | ${TODAY} | AI Agent | 架构师 | 初始化任务操作指南脚手架 |

---

## 1. 目标与场景 (Objective)
说明本指南解决的具体生产任务或故障排查目标。

## 2. 前置条件 (Prerequisites)
- 执行前必须就绪的凭证、权限或环境变量

## 3. 标准操作规程 (SOP / Execution Steps)
### 3.1 步骤一
\`\`\`bash
# 可直接执行的命令
\`\`\`

### 3.2 步骤二
\`\`\`bash
# 可直接执行的命令
\`\`\`

## 4. 验证与排错 (Verification & Troubleshooting)
- 如何验证操作成功（预期状态码与输出日志）
- 常见偶发问题与对策
EOF
    ;;

  reference)
    TARGET_DIR="$ROOT_DIR/reference"
    mkdir -p "$TARGET_DIR"
    FILE_PATH="$TARGET_DIR/${NAME}.md"
    cat <<EOF > "$FILE_PATH"
# ${NAME} 权威技术参考 (Technical Reference)

> **文档控制信息**
> - **文档标识**: REF-${NAME^^}-${YEAR}
> - **当前版本**: V1.0.0
> - **文档状态**: Active
> - **生效日期**: ${TODAY}
> - **文档所有者**: 核心架构组

---

### 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | ${TODAY} | AI Agent | 架构师 | 初始化权威技术参考脚手架 |

---

> ⚠️ **机器真理源声明**: 本文档为全系统权威机械规格唯一事实来源，供 AI 智能体生成代码与人机对齐使用。

## 1. 接口与数据模型契约 (Schema & API Contracts)

### 1.1 数据结构定义
| 字段名 | 类型 | 必填 | 默认值 | 约束说明 |
| :--- | :--- | :--- | :--- | :--- |
| \`id\` | string | 是 | 无 | 唯一实体 UUID |
| \`status\` | string | 是 | "pending" | 枚举: pending, running, completed |

### 1.2 异常与错误信封 (Error Envelope)
\`\`\`json
{
  "code": "ERR_INVALID_PAYLOAD",
  "message": "请求载荷字段校验失败",
  "details": {}
}
\`\`\`

## 2. 状态机与不可变约束 (Invariants)
- 状态转换规则与守卫条件
EOF
    ;;

  explanation)
    TARGET_DIR="$ROOT_DIR/explanation"
    mkdir -p "$TARGET_DIR"
    FILE_PATH="$TARGET_DIR/${NAME}.md"
    cat <<EOF > "$FILE_PATH"
# ${NAME} 深度剖析与架构设计 (Explanation & Architecture)

> **文档控制信息**
> - **文档标识**: EXP-${NAME^^}-${YEAR}
> - **当前版本**: V1.0.0
> - **文档状态**: Active
> - **生效日期**: ${TODAY}
> - **文档所有者**: 架构设计委员会

---

### 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | ${TODAY} | AI Agent | 架构师 | 初始化系统深度剖析脚手架 |

---

## 1. 设计哲学与宏观背景 (Philosophy & Context)
阐述系统为什么如此设计，以及核心第一性原理。

## 2. 总体架构拓扑 (Architecture Topology)
\`\`\`mermaid
graph TD
    Client["表现层 / Client"] --> Core["核心领域业务 / Core"]
    Core --> Store["不可变状态与存储 / Store"]
\`\`\`

## 3. 核心机制推演 (Mechanism & Trade-offs)
- 核心算法推导
- 边界并发竞争与防护机理
EOF
    ;;

  adr)
    TARGET_DIR="$ROOT_DIR/explanation/decisions"
    mkdir -p "$TARGET_DIR"
    # 计算当前最大序号
    NEXT_SEQ=1
    EXISTING_ADRS=$(find "$TARGET_DIR" -maxdepth 1 -name "????-*.md" 2>/dev/null | sort || true)
    if [ -n "$EXISTING_ADRS" ]; then
      LAST_FILE=$(basename "$(echo "$EXISTING_ADRS" | tail -n 1)")
      LAST_NUM=$(echo "$LAST_FILE" | cut -d'-' -f1 | sed 's/^0*//')
      if [ -n "$LAST_NUM" ]; then
        NEXT_SEQ=$((LAST_NUM + 1))
      fi
    fi
    SEQ_PADDED=$(printf "%04d" "$NEXT_SEQ")
    FILE_PATH="$TARGET_DIR/${SEQ_PADDED}-${NAME}.md"
    cat <<EOF > "$FILE_PATH"
# ADR-${SEQ_PADDED}: ${NAME}

> **ADR 控制元数据**
> - **决策编号**: ${SEQ_PADDED}
> - **当前状态**: Proposed
> - **决策所有者**: 架构组
> - **决议日期**: ${TODAY}

---

## 1. 背景与上下文 (Context and Problem Statement)
- 面临的现实技术或业务问题是什么？
- 有哪些硬性约束？

## 2. 考虑的备选方案 (Considered Options)
* **方案 1**: 备选方案 A
* **方案 2 (选定)**: 备选方案 B
* **方案 3**: 备选方案 C

## 3. 最终决策与裁决 (Decision Outcome)
选定 **方案 2**，原因是：核心优势明显且契合项目技术栈。

### 3.1 积极后果 (Positive Consequences)
* ✅ 优势 1: ...

### 3.2 消极后果与已知代价 (Negative Consequences / Trade-offs)
* ⚠️ 代价 1: ...

---

## 4. 备选方案详细对比 (Pros and Cons)
### 4.1 方案 1
* 赞同理由 (+): ...
* 反对理由 (-): ...

### 4.2 方案 2 (选定)
* 赞同理由 (+): ...
EOF
    ;;

  rfc|proposal)
    TARGET_DIR="$ROOT_DIR/proposals"
    mkdir -p "$TARGET_DIR"
    NEXT_SEQ=1
    EXISTING_RFCS=$(find "$TARGET_DIR" -maxdepth 1 -name "RFC-????-*.md" 2>/dev/null | sort || true)
    if [ -n "$EXISTING_RFCS" ]; then
      LAST_FILE=$(basename "$(echo "$EXISTING_RFCS" | tail -n 1)")
      LAST_NUM=$(echo "$LAST_FILE" | cut -d'-' -f2 | sed 's/^0*//')
      if [ -n "$LAST_NUM" ]; then
        NEXT_SEQ=$((LAST_NUM + 1))
      fi
    fi
    SEQ_PADDED=$(printf "%04d" "$NEXT_SEQ")
    FILE_PATH="$TARGET_DIR/RFC-${SEQ_PADDED}-${NAME}.md"
    cat <<EOF > "$FILE_PATH"
# RFC-${SEQ_PADDED}: ${NAME}

> **提案元数据**
> - **标识**: RFC-${YEAR}-$(echo "$NAME" | tr '[:lower:]' '[:upper:]')
> - **当前状态**: Draft
> - **发起人**: 工程师 / AI Agent
> - **当前版本**: V0.1.0
> - **初次发起日期**: ${TODAY}
> - **目标里程碑**: [如 v0.7.0]

---

## 1. 背景与业务痛点 (Why)
- 为什么需要引入此特性？当前痛点是什么？

## 2. 目标与非目标 (Goals & Non-Goals)
### 2.1 核心目标 (Goals)
- [ ] 目标 1: ...
### 2.2 明确非目标 (Non-Goals)
- 明确本期坚决不做的边界

## 3. 核心设计与契约草案 (How - Draft Spec)
> ⚠️ 本节内容为初期探索草案，定案前严禁作为生产基线直接引用。
### 3.1 交互流程与用户故事
### 3.2 领域数据模型草案 (Draft Models)
### 3.3 外部接口与事件草案 (Draft APIs)

## 4. 备选方案权衡与争议焦点 (Alternatives & Trade-offs)
- **方案 A (选定草案)**:
- **方案 B (备选方案)**:

## 5. 未决问题与对齐清单 (Open Questions)
- [ ] 焦点 1: ...
- [ ] 焦点 2: ...
EOF
    ;;

  *)
    echo "❌ 错误: 未知类型 '$TYPE'。可用类型: tutorial, how-to, reference, explanation, adr, rfc"
    exit 1
    ;;
esac

echo "✅ 成功生成 $TYPE 模板脚手架: $FILE_PATH"
