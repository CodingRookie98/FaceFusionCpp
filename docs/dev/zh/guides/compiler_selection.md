# 编译器选型技术决策记录 (Compiler Selection ADR)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-GUIDE-COMPILER-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |


> **文档标识**: FACE-FUSION-ADR-COMPILER
> **决策日期**: 2026-05-29
> **状态**: 已采纳 (Accepted)
> **决策人**: hui + hermes

---

## 1. 背景 (Context)

项目 faceFusionCpp 是一个重度依赖 C++20 Modules 的高性能人脸处理管线，当前使用 LLVM/Clang 工具链。随着 GCC 16 发布（默认 C++20 标准）和 Clang 22 发布（最新稳定版），需要评估编译器升级或切换的必要性。

### 当前环境

| 组件 | 版本 | 说明 |
| :--- | :--- | :--- |
| **Clang (当前)** | 21.1.8 | Ubuntu LLVM PPA |
| **Clang (最新稳定)** | 22.1.0 | 2026-02-24 发布 |
| **GCC (系统)** | 13.3.0 | Ubuntu 24.04 默认 |
| **GCC (最新)** | 16.x | 2026 年发布 |
| **LLVM 工具链** | 21.1.8 | clangd, clang-tidy, clang-format, lld |
| **vcpkg baseline** | `6d7bf7ef...` | 锁定的依赖版本 |

### 项目 C++20 Modules 规模

| 指标 | 数量 |
| :--- | :--- |
| `.ixx` 接口模块 | 107 |
| `.cpp` 实现文件 | 68 |
| `export module` 声明 | 124 |
| 模块化覆盖率 | 98.9% |

---

## 2. 决策 (Decision)

**保持当前 Clang 21.1.8，不升级，不切换 GCC。**

---

## 3. 方案对比 (Options Considered)

### 方案 A：继续 Clang 21（采纳 ✅）

| 维度 | 评估 |
| :--- | :--- |
| C++20 支持率 | 95.6% |
| C++23 支持率 | ~89% |
| Modules 稳定性 | ✅ 124 模块已验证 |
| 第三方库兼容性 | ✅ 已解决（patch_opencv.py） |
| 工具链完整性 | ✅ clangd + clang-tidy + clang-format |
| 迁移成本 | 零 |

### 方案 B：升级到 Clang 22（否决 ❌）

| 维度 | 评估 |
| :--- | :--- |
| C++20 支持率 | 95.6%（无差异） |
| C++23 支持率 | 91.1%（+2%） |
| Modules 稳定性 | ❓ 需重新验证 |
| 第三方库兼容性 | ❌ 高风险（OpenCV, FFmpeg, OpenSSL 等） |
| 工具链完整性 | ✅ 同步升级 |
| 迁移成本 | **高**（依赖链全面重测） |

**否决理由**：

1. **第三方库兼容性风险**：Clang 版本升级会导致编译器更严格，OpenCV、FFmpeg、OpenSSL 等第三方库可能编译失败。已知问题：
   - OpenCV `types.hpp` 的 C++20 模板歧义（已通过 `patch_opencv.py` 修复）
   - FFmpeg 的某些 deprecated 用法
   - vcpkg 的包版本是针对特定编译器版本测试的，升级编译器可能需要更新 vcpkg baseline，引发连锁反应

2. **收益不足**：C++23 支持率仅提升 2%（89% → 91.1%），不值得冒迁移风险

3. **无明确驱动因素**：当前没有必须使用 Clang 22 才支持的 C++23 特性

### 方案 C：切换到 GCC 16（否决 ❌）

| 维度 | 评估 |
| :--- | :--- |
| C++20 支持率 | 完全（默认标准） |
| C++23 支持率 | ~79% |
| Modules 稳定性 | ❌ **实验性**（需 `-fmodules`） |
| 第三方库兼容性 | ⚠️ 需全面重测 |
| 工具链完整性 | ❌ 失去 clangd, clang-tidy |
| 迁移成本 | **极高** |

**否决理由**：

1. **Modules 仍为实验性**：GCC 16 官方明确声明 "C++20 modules support is still experimental and must be enabled by -fmodules"。项目已有 124 个模块，迁移风险不可接受。

2. **工具链生态断裂**：
   - GCC 没有等价的 clangd（LSP），IDE 智能提示能力丧失
   - GCC 没有等价的 clang-tidy 集成，静态分析需要寻找替代方案
   - 即使切换编译器，仍需保留 LLVM 工具链，形成工具链分裂

3. **C++23 覆盖率更低**：GCC 16 的 C++23 支持率（79%）低于 Clang 21（89%）

4. **GCC 16 适合的场景**（本项目均不属于）：
   - 嵌入式/交叉编译（ARM, RISC-V）
   - Linux 内核/系统级开发
   - 不使用 Modules 的传统项目
   - 需要 GCC 特有优化的场景

---

## 4. 升级触发条件 (When to Reconsider)

升级编译器的触发条件（满足以下任一即可重新评估）：

| 条件 | 说明 |
| :--- | :--- |
| **C++23 特性成为必须** | 需要 `std::expected` 或 `std::generator`，且 Clang 21 不支持 |
| **安全漏洞** | Clang 21 有已知安全漏洞且无补丁 |
| **第三方库要求** | 核心依赖（如 ONNX Runtime）要求更高版本编译器 |
| **Clang 21 EOL** | LLVM 21 不再有安全更新 |

当前状态：**四个条件均不满足**。

---

## 5. 未来升级预案 (Upgrade Playbook)

如果未来必须升级，按以下步骤执行：

```bash
# 1. 创建隔离的测试分支
git checkout -b experiment/clang-22

# 2. 更新 vcpkg baseline
# 修改 vcpkg.json 的 builtin-baseline 到最新 commit

# 3. 重新编译所有依赖
vcpkg install --triplet x64-linux

# 4. 运行 patch 脚本（可能需要新增 patch）
python scripts/patch_opencv.py

# 5. 全量编译 + 测试
python build.py --action configure
python build.py --action build
python build.py --action test --test-label unit
python build.py --action test --test-label integration

# 6. 如果失败 > 3 个库，放弃升级
```

---

## 6. 附录：编译器支持对比

### C++20 核心特性

| 特性 | Clang 21 | Clang 22 | GCC 16 |
| :--- | :--- | :--- | :--- |
| **Modules** | ✅ 稳定 | ✅ 稳定 | ⚠️ 实验性 |
| **Concepts** | ✅ | ✅ | ✅ |
| **Ranges** | ✅ | ✅ | ✅ |
| **Coroutines** | ✅ (Linux) | ✅ (Linux) | ✅ |
| **std::format** | ✅ | ✅ | ✅ |
| **std::span** | ✅ | ✅ | ✅ |
| **<=> 三路比较** | ✅ | ✅ | ✅ |

### C++23 关键特性

| 特性 | Clang 21 | Clang 22 | GCC 16 |
| :--- | :--- | :--- | :--- |
| **std::expected** | ✅ | ✅ | ✅ |
| **std::mdspan** | ✅ | ✅ | ✅ |
| **std::print** | ✅ | ✅ | ✅ |
| **std::generator** | ✅ | ✅ | ✅ |
| **deducing this** | ✅ | ✅ | ✅ |
| **std::flat_map** | ✅ | ✅ | ✅ |
| **std::stacktrace** | ✅ | ✅ | ✅ |

### 工具链生态

| 工具 | LLVM 生态 | GCC 生态 |
| :--- | :--- | :--- |
| 编译器 | clang++ | g++ |
| LSP | ✅ clangd | ❌ 无原生方案 |
| 静态分析 | ✅ clang-tidy | ⚠️ cppcheck |
| 格式化 | ✅ clang-format | ✅ 可独立使用 |
| 链接器 | lld | ld |
| Sanitizer | ✅ ASan/MSan/TSan | ✅ ASan/TSan |

---

## 7. 附录：已知第三方库兼容性问题

### OpenCV (已解决)

**问题**：`opencv2/core/types.hpp` 中的 `Rect` 类在 C++20 下存在模板歧义。

**解决**：`scripts/patch_opencv.py` 自动修补 `types.hpp`，将 `a = Rect()` 改为 `a = Rect_<_Tp>()`。

**集成**：CMakeLists.txt 中已集成自动 patch 流程。

### 潜在风险库

| 库 | 风险等级 | 说明 |
| :--- | :--- | :--- |
| OpenCV | 🔴 高 | 已知 C++20 歧义，已 patch |
| FFmpeg | 🟡 中 | C 代码库，deprecated 用法 |
| OpenSSL | 🟡 中 | 大量平台特定代码 |
| ONNX Runtime | 🟢 低 | 官方支持 Clang |
| spdlog | 🟢 低 | 现代 C++，兼容性好 |
| yaml-cpp | 🟢 低 | 兼容性好 |
| nlohmann-json | 🟢 低 | 兼容性好 |

---

## 8. 相关文档

- `AGENTS.md` — 项目开发规范
- `vcpkg.json` — 依赖管理配置
