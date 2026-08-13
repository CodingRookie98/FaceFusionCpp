# CLI 增强设计方案：处理器参数动态注册

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-SUPER-SPEC-CLI-2026
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


> **文档标识**: FACE-FUSION-CLI-ENHANCE
> **状态**: 设计确认
> **创建日期**: 2026-05-29
> **设计驱动**: 脚本/自动化优先

## 1. 问题陈述

当前 CLI 只能通过 `--processors face_swapper,face_enhancer` 选择处理器名称，**无法指定任何处理器参数**（如 model、blend_factor、face_selector_mode）。用户必须手写 YAML 才能调参，CLI 快捷模式形同虚设。

**设计文档要求但未实现的能力**：
- 处理器细粒度参数通过 CLI 暴露（`--validate` + 快捷模式）
- 每个 processor 有独立参数（model, blend_factor, face_selector_mode, reference_face_path 等）

## 2. 设计决策

| 决策项 | 选择 | 理由 |
|--------|------|------|
| 使用场景 | 脚本/自动化优先 | CI/CD 管道、批处理脚本中精确控制参数 |
| 命名风格 | 带处理器前缀 | 无歧义，脚本友好 |
| 参数注册 | 元数据注册 + CLI 动态生成 | 单一事实来源，维护成本低 |
| `--validate` | 统一校验路径 | 快捷模式合成 TaskConfig 后走同一校验流程 |

## 3. 架构设计

### 3.1 处理器参数注册表

核心抽象：每个处理器通过静态注册宏声明参数元数据，CLI 从注册表动态生成 flag。

```cpp
// 参数类型枚举
enum class ParamType { String, Int, Float, Bool, Path };

// 单个参数的元数据
struct ParamMeta {
    std::string name;                              // 参数名（snake_case）
    ParamType type;                                // 类型
    std::string default_value;                     // 默认值（字符串表示）
    std::vector<std::string> allowed_values;       // 枚举允许值（空 = 不限）
    std::string description;                       // 描述
    std::optional<std::pair<double, double>> range; // 数值范围（仅 Int/Float）
};

// 处理器注册信息
struct ProcessorMeta {
    std::string name;                    // 处理器名（如 "face_swapper"）
    std::vector<ParamMeta> params;       // 参数列表
};

// 全局注册表（单例）
class ProcessorParamRegistry {
public:
    static ProcessorParamRegistry& instance();

    void register_processor(const ProcessorMeta& meta);
    const ProcessorMeta* find(const std::string& name) const;
    std::vector<std::string> all_processor_names() const;
    bool is_valid_processor(const std::string& name) const;
};
```

**注册宏**（在处理器 `.cpp` 文件中使用）：

```cpp
// face_swapper.cpp
REGISTER_PROCESSOR_PARAMS("face_swapper", {
    {"model",              ParamType::String, "inswapper_128_fp16",
        {"inswapper_128", "inswapper_128_fp16"}, "Swap model name"},
    {"face_selector_mode", ParamType::String, "many",
        {"reference", "one", "many"}, "Face selection mode"},
    {"reference_face_path",ParamType::Path,   "",
        {}, "Reference face image (required if mode=reference)"},
});

// face_enhancer.cpp
REGISTER_PROCESSOR_PARAMS("face_enhancer", {
    {"model",              ParamType::String, "codeformer",
        {"codeformer", "gfpgan_1.2", "gfpgan_1.3", "gfpgan_1.4"}, "Enhancer model"},
    {"blend_factor",       ParamType::Float,  "0.8",
        {}, "Blend factor", std::make_pair(0.0, 1.0)},
    {"face_selector_mode", ParamType::String, "many",
        {"reference", "one", "many"}, "Face selection mode"},
    {"reference_face_path",ParamType::Path,   "",
        {}, "Reference face image"},
});

// expression_restorer.cpp
REGISTER_PROCESSOR_PARAMS("expression_restorer", {
    {"model",              ParamType::String, "live_portrait",
        {"live_portrait"}, "Restorer model"},
    {"restore_factor",     ParamType::Float,  "0.8",
        {}, "Restore factor", std::make_pair(0.0, 1.0)},
    {"face_selector_mode", ParamType::String, "many",
        {"reference", "one", "many"}, "Face selection mode"},
    {"reference_face_path",ParamType::Path,   "",
        {}, "Reference face image"},
});

// frame_enhancer.cpp
REGISTER_PROCESSOR_PARAMS("frame_enhancer", {
    {"model",              ParamType::String, "real_esrgan_x4",
        {"real_esrgan_x2", "real_esrgan_x2_fp16", "real_esrgan_x4",
         "real_esrgan_x4_fp16", "real_esrgan_x8", "real_esrgan_x8_fp16",
         "real_hatgan_x4"}, "Frame enhancer model"},
    {"enhance_factor",     ParamType::Float,  "0.8",
        {}, "Enhance factor", std::make_pair(0.0, 1.0)},
});
```

**模块位置**：
- 注册表接口：`src/domain/processor/processor_param_registry.ixx`
- 注册表实现：`src/domain/processor/processor_param_registry.cpp`
- 注册宏定义：`src/domain/processor/processor_param_registry.ixx`

### 3.2 CLI 动态注册

CLI 启动时从注册表动态生成处理器参数 flag。

**命名转换规则**：`{processor_name}_{param_name}` → `--{processor-name}-{param-name}`

| 处理器 | 参数 | CLI flag |
|--------|------|----------|
| `face_swapper` | `model` | `--face-swapper-model` |
| `face_swapper` | `face_selector_mode` | `--face-swapper-face-selector-mode` |
| `face_swapper` | `reference_face_path` | `--face-swapper-reference-face-path` |
| `face_enhancer` | `model` | `--face-enhancer-model` |
| `face_enhancer` | `blend_factor` | `--face-enhancer-blend-factor` |
| `expression_restorer` | `restore_factor` | `--expression-restorer-restore-factor` |
| `frame_enhancer` | `enhance_factor` | `--frame-enhancer-enhance-factor` |

**类型映射**：

| ParamType | CLI11 方法 | 校验 |
|-----------|-----------|------|
| `String` | `add_option` | `->check(CLI::IsMember({...}))` 若有枚举 |
| `Int` | `add_option` | `->check(CLI::Range(min, max))` 若有范围 |
| `Float` | `add_option` | `->check(CLI::Range(min, max))` 若有范围 |
| `Bool` | `add_flag` | — |
| `Path` | `add_option` | — |

**参数收集**：
- 解析后得到 `std::map<std::string, std::map<std::string, std::string>>`
- key1 = processor_name, key2 = param_name, value = 用户指定值
- 仅记录用户**显式指定**的参数（未指定的不写入 map）

**CLI 帮助输出分组**：

```
GLOBAL OPTIONS:
  -h, --help              ...
  -c, --task-config TEXT  ...
  ...

QUICK MODE OPTIONS:
  -s, --source TEXT ...   Source face image(s)
  -t, --target TEXT ...   Target image/video path(s)
  -o, --output TEXT       Output path
  --processors TEXT       Processor list

PROCESSOR OPTIONS (Quick Mode only):
  --face-swapper-model TEXT:{inswapper_128,inswapper_128_fp16}
  --face-swapper-face-selector-mode TEXT:{reference,one,many}
  --face-swapper-reference-face-path TEXT
  --face-enhancer-model TEXT:{codeformer,gfpgan_1.2,...}
  --face-enhancer-blend-factor FLOAT in [0 - 1]
  ...
```

### 3.3 快捷模式增强

**合成流程**：

```
CLI 参数 ──→ CLI11 解析 ──→ 合成 TaskConfig ──→ 校验 ──→ 执行
                               │                    │
                               │                    └─ --validate 时在此停止
                               │
                               ├─ task_info.id = 自动生成 UUID
                               ├─ io.source_paths = {-s 参数}
                               ├─ io.target_paths = {-t 参数}
                               ├─ io.output.path = {-o 参数} 或默认 "./output/"
                               ├─ pipeline = 从 --processors + 显式参数生成
                               │   └─ 每个 step.params = 仅用户显式指定的参数
                               └─ 未指定参数由 ConfigMerger 填充默认值
```

**`run_quick_mode` 增强**：

```cpp
int App::run_quick_mode(
    const std::vector<std::string>& source_paths,
    const std::vector<std::string>& target_paths,
    const std::string& output_path,
    const std::string& processors_str,
    const std::map<std::string, std::map<std::string, std::string>>& processor_params, // 新增
    const config::AppConfig& app_config)
{
    // 1. 构建 TaskConfig（同现有逻辑）
    // ...

    // 2. 解析 processors
    for (const auto& proc : processor_names) {
        PipelineStep step;
        step.step = proc;
        step.enabled = true;

        // 3. 注入用户显式指定的参数
        auto it = processor_params.find(proc);
        if (it != processor_params.end()) {
            for (const auto& [param_name, param_value] : it->second) {
                step.params[param_name] = param_value;
            }
        }

        task_config.pipeline.push_back(step);
    }

    // 4. 合并默认值 + 执行
    task_config = MergeConfigs(task_config, app_config);
    return run_pipeline_internal(task_config, app_config);
}
```

### 3.4 `--validate` 统一校验路径

**修改前**：
```cpp
if (validate_only) {
    if (config_path.empty()) {
        std::cerr << "Error: --validate requires --task-config" << '\n';
        exit_code = 1;
    } else {
        exit_code = run_validate(config_path, *app_config);
    }
}
```

**修改后**：
```cpp
if (validate_only) {
    std::optional<config::TaskConfig> task_config;
    if (!config_path.empty()) {
        task_config = load_task_config(config_path);
    } else if (!source_paths.empty() && !target_paths.empty()) {
        task_config = build_quick_task_config(source_paths, target_paths,
                                               output_path, processors_str,
                                               processor_params);
    } else {
        std::cerr << "Error: --validate requires --task-config or (-s, -t)\n";
        exit_code = 1;
    }
    if (exit_code == 0 && task_config) {
        exit_code = run_validate(*task_config, *app_config);
    }
}
```

**校验输出格式**（与 YAML 校验一致）：
```
[OK] Task ID: quick_a1b2c3d4
[OK] Source paths: 1 file(s) found
[OK] Target paths: 1 file(s) found
[OK] Output path: /tmp/test/
[OK] Pipeline step 1: face_swapper (model=inswapper_128_fp16)
[WARN] Pipeline step 2: face_enhancer - blend_factor=1.5 out of range [0.0, 1.0]
---
Result: 0 FAIL, 1 WARN, 0 ERROR
```

## 4. 错误处理

| 场景 | 处理方式 |
|------|---------|
| 未知处理器名（`--processors foo`） | 报错 + 列出可用处理器 |
| 参数值越界（`--face-enhancer-blend-factor 2.0`） | CLI11 自动报错 + 提示范围 |
| 枚举值不合法（`--face-swapper-model xxx`） | CLI11 自动报错 + 列出可选值 |
| `--face-swapper-reference-face-path` 指定但 mode≠reference | 运行时 WARN（非致命） |
| 快捷模式缺少 `-s` 或 `-t` | 报错提示必填参数 |
| `--validate` 缺少 `-c` 和 `-s/-t` | 报错提示需要配置来源 |

## 5. 测试策略

| 测试层级 | 测试内容 | 优先级 |
|---------|---------|--------|
| 单元测试 | ProcessorParamRegistry 注册/查找/枚举 | P0 |
| 单元测试 | build_quick_task_config 合成逻辑 | P0 |
| 单元测试 | 参数类型校验（范围、枚举、必填） | P0 |
| 集成测试 | CLI 动态注册 → 解析 → 合成 → 校验 全链路 | P0 |
| E2E 测试 | 快捷模式 + 处理器参数 → 实际执行 → 验证输出 | P1 |

## 6. 范围边界

### 包含（本次实现）

- 处理器参数元数据注册表（ProcessorParamRegistry + 注册宏）
- CLI 动态 flag 生成（从注册表自动生成 CLI11 参数）
- 快捷模式参数注入（`run_quick_mode` 读取显式参数）
- `--validate` 快捷模式支持（统一校验路径）
- 错误提示优化（未知处理器、参数越界、缺少必填参数）

### 不包含（未来再做）

- 子命令模式（`FaceFusionCpp run/validate/batch`）
- `--verbose` 全局开关
- YAML 配置覆盖单个参数（`--override`）
- Shell 自动补全脚本生成
- 配置迁移工具

## 7. CLI 最终用法示例

```bash
# 1. 快捷模式：最简用法（全部默认参数）
FaceFusionCpp -s source.jpg -t target.jpg -o output/

# 2. 快捷模式：指定处理器 + 参数
FaceFusionCpp \
  -s source.jpg \
  -t target.mp4 \
  -o output/ \
  --processors face_swapper,face_enhancer \
  --face-swapper-model inswapper_128_fp16 \
  --face-swapper-face-selector-mode reference \
  --face-swapper-reference-face-path ref.jpg \
  --face-enhancer-blend-factor 0.9

# 3. 快捷模式 + validate（预检）
FaceFusionCpp \
  -s source.jpg \
  -t target.jpg \
  --processors face_swapper \
  --face-swapper-model inswapper_128 \
  --validate

# 4. YAML 模式（不变）
FaceFusionCpp -c task_config.yaml

# 5. 系统检查
FaceFusionCpp --system-check --json
```
