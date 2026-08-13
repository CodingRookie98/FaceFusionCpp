# CLI Enhancement: Processor Parameter Dynamic Registration

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-PLAN-CLI-2026
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


> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Expose processor-specific parameters via CLI flags in quick mode, enabling script/automation usage without writing YAML.

**Architecture:** Introduce a `ProcessorParamRegistry` singleton (mirroring the existing `ProcessorFactory` pattern) where each processor declares its parameter metadata. The CLI dynamically generates CLI11 flags from this registry. Quick mode synthesizes a `TaskConfig` from CLI params, and `--validate` works uniformly for both YAML and quick modes.

**Tech Stack:** C++20 modules, CLI11 (already integrated), GTest/GMock (existing test framework)

**Design Spec:** `docs/superpowers/specs/2026-05-29-cli-enhancement-design.md`

---

## Task 1: Create ProcessorParamRegistry module (interface + types)

**Objective:** Define the parameter metadata types and registry interface as a C++20 module.

**Files:**
- Create: `src/domain/processor/processor_param_registry.ixx`
- Test: `tests/unit/domain/processor/processor_param_registry_test.cpp`

**Step 1: Write failing test**

```cpp
// tests/unit/domain/processor/processor_param_registry_test.cpp
#include <gtest/gtest.h>
#include <string>
#include <vector>

import processor.param_registry;

using namespace domain::processor;

class ProcessorParamRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test uses unique processor names to avoid singleton pollution
    }
};

TEST_F(ProcessorParamRegistryTest, RegisterAndFindProcessor) {
    std::string name = "test_proc_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor({
        .name = name,
        .params = {
            {"model", ParamType::String, "default_model", {"model_a", "model_b"}, "Model name"},
            {"factor", ParamType::Float, "0.8", {}, "Blend factor", std::make_pair(0.0, 1.0)},
        }
    });

    auto* meta = ProcessorParamRegistry::instance().find(name);
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->name, name);
    EXPECT_EQ(meta->params.size(), 2u);
    EXPECT_EQ(meta->params[0].name, "model");
    EXPECT_EQ(meta->params[0].type, ParamType::String);
    EXPECT_EQ(meta->params[0].default_value, "default_model");
    EXPECT_EQ(meta->params[0].allowed_values, (std::vector<std::string>{"model_a", "model_b"}));
    EXPECT_EQ(meta->params[1].name, "factor");
    EXPECT_EQ(meta->params[1].type, ParamType::Float);
    EXPECT_TRUE(meta->params[1].range.has_value());
    EXPECT_DOUBLE_EQ(meta->params[1].range->first, 0.0);
    EXPECT_DOUBLE_EQ(meta->params[1].range->second, 1.0);
}

TEST_F(ProcessorParamRegistryTest, FindUnknownReturnsNull) {
    auto* meta = ProcessorParamRegistry::instance().find("nonexistent_proc_xyz");
    EXPECT_EQ(meta, nullptr);
}

TEST_F(ProcessorParamRegistryTest, AllProcessorNamesIncludesRegistered) {
    std::string name = "test_list_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor({.name = name, .params = {}});

    auto names = ProcessorParamRegistry::instance().all_processor_names();
    EXPECT_NE(std::find(names.begin(), names.end(), name), names.end());
}

TEST_F(ProcessorParamRegistryTest, IsValidProcessor) {
    std::string name = "test_valid_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor({.name = name, .params = {}});

    EXPECT_TRUE(ProcessorParamRegistry::instance().is_valid_processor(name));
    EXPECT_FALSE(ProcessorParamRegistry::instance().is_valid_processor("no_such_proc_abc"));
}
```

**Step 2: Run test to verify failure**

Run: `python build.py --action test --test-label unit`
Expected: Compilation failure — `processor.param_registry` module not found.

**Step 3: Write the module interface**

```cpp
// src/domain/processor/processor_param_registry.ixx
module;

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <algorithm>

export module processor.param_registry;

export namespace domain::processor {

enum class ParamType { String, Int, Float, Bool, Path };

struct ParamMeta {
    std::string name;
    ParamType type;
    std::string default_value;
    std::vector<std::string> allowed_values;
    std::string description;
    std::optional<std::pair<double, double>> range;
};

struct ProcessorMeta {
    std::string name;
    std::vector<ParamMeta> params;
};

class ProcessorParamRegistry {
public:
    static ProcessorParamRegistry& instance() {
        static ProcessorParamRegistry registry;
        return registry;
    }

    void register_processor(ProcessorMeta meta) {
        m_registry[meta.name] = std::move(meta);
    }

    [[nodiscard]] const ProcessorMeta* find(const std::string& name) const {
        auto it = m_registry.find(name);
        return it != m_registry.end() ? &it->second : nullptr;
    }

    [[nodiscard]] std::vector<std::string> all_processor_names() const {
        std::vector<std::string> names;
        names.reserve(m_registry.size());
        for (const auto& [k, v] : m_registry) {
            names.push_back(k);
        }
        return names;
    }

    [[nodiscard]] bool is_valid_processor(const std::string& name) const {
        return m_registry.contains(name);
    }

private:
    ProcessorParamRegistry() = default;
    std::map<std::string, ProcessorMeta> m_registry;
};

// Helper struct for static registration (mirrors ProcessorRegistrar pattern)
export struct ProcessorParamRegistrar {
    ProcessorParamRegistrar(ProcessorMeta meta) {
        ProcessorParamRegistry::instance().register_processor(std::move(meta));
    }
};

} // namespace domain::processor
```

**Step 4: Add to CMakeLists**

Add `src/domain/processor/processor_param_registry.ixx` to the appropriate `FILE_SET cxx_modules` in `src/domain/processor/CMakeLists.txt` (or parent CMakeLists if the directory doesn't exist yet).

Add the test file to `tests/CMakeLists.txt` under the unit test target.

**Step 5: Run test to verify pass**

Run: `python build.py --action test --test-label unit`
Expected: All 4 new tests PASS.

**Step 6: Commit**

```bash
git add src/domain/processor/processor_param_registry.ixx tests/unit/domain/processor/processor_param_registry_test.cpp
git commit -m "feat(processor): add ProcessorParamRegistry module with metadata types"
```

---

## Task 2: Register face_swapper parameters

**Objective:** Register face_swapper's parameter metadata using the ProcessorParamRegistrar.

**Files:**
- Modify: `src/domain/face/swapper/impl/inswapper.cpp` (or the file where face_swapper registers with ProcessorFactory)
- Test: Extend `tests/unit/domain/processor/processor_param_registry_test.cpp`

**Step 1: Write failing test**

```cpp
// Add to processor_param_registry_test.cpp
TEST_F(ProcessorParamRegistryTest, FaceSwapperIsRegistered) {
    auto* meta = ProcessorParamRegistry::instance().find("face_swapper");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->name, "face_swapper");
    // Should have at least model, face_selector_mode, reference_face_path
    EXPECT_GE(meta->params.size(), 3u);

    // Verify model param
    auto* model_param = find_param(meta, "model");
    ASSERT_NE(model_param, nullptr);
    EXPECT_EQ(model_param->type, ParamType::String);
    EXPECT_FALSE(model_param->allowed_values.empty());
}

// Helper (add to test file or a shared test utility)
const ParamMeta* find_param(const ProcessorMeta* meta, const std::string& name) {
    for (const auto& p : meta->params) {
        if (p.name == name) return &p;
    }
    return nullptr;
}
```

**Step 2: Run test to verify failure**

Expected: `face_swapper` not found in registry.

**Step 3: Add registration to inswapper.cpp**

Find where `ProcessorRegistrar` is used for face_swapper and add the param registration alongside it:

```cpp
// At file scope, near the existing ProcessorRegistrar usage
static domain::processor::ProcessorParamRegistrar face_swapper_params_{
    .name = "face_swapper",
    .params = {
        {"model", domain::processor::ParamType::String, "inswapper_128_fp16",
            {"inswapper_128", "inswapper_128_fp16"}, "Swap model name"},
        {"face_selector_mode", domain::processor::ParamType::String, "many",
            {"reference", "one", "many"}, "Face selection mode"},
        {"reference_face_path", domain::processor::ParamType::Path, "",
            {}, "Reference face image (required if mode=reference)"},
    }
};
```

**Step 4: Run test to verify pass**

Run: `python build.py --action test --test-label unit`
Expected: All tests PASS including `FaceSwapperIsRegistered`.

**Step 5: Commit**

```bash
git add src/domain/face/swapper/impl/inswapper.cpp tests/unit/domain/processor/processor_param_registry_test.cpp
git commit -m "feat(processor): register face_swapper parameter metadata"
```

---

## Task 3: Register remaining processors (face_enhancer, expression_restorer, frame_enhancer)

**Objective:** Register parameter metadata for all 3 remaining processors.

**Files:**
- Modify: `src/domain/face/enhancer/impl/codeformer.cpp` (or gfp_gan.cpp — whichever registers face_enhancer)
- Modify: Expression restorer registration file
- Modify: `src/domain/frame/enhancer/impl/frame_enhancer_impl.cpp`
- Test: Extend test file

**Step 1: Write failing tests**

```cpp
TEST_F(ProcessorParamRegistryTest, FaceEnhancerIsRegistered) {
    auto* meta = ProcessorParamRegistry::instance().find("face_enhancer");
    ASSERT_NE(meta, nullptr);
    EXPECT_GE(meta->params.size(), 3u); // model, blend_factor, face_selector_mode
}

TEST_F(ProcessorParamRegistryTest, ExpressionRestorerIsRegistered) {
    auto* meta = ProcessorParamRegistry::instance().find("expression_restorer");
    ASSERT_NE(meta, nullptr);
    EXPECT_GE(meta->params.size(), 2u); // model, restore_factor
}

TEST_F(ProcessorParamRegistryTest, FrameEnhancerIsRegistered) {
    auto* meta = ProcessorParamRegistry::instance().find("frame_enhancer");
    ASSERT_NE(meta, nullptr);
    EXPECT_GE(meta->params.size(), 2u); // model, enhance_factor
}
```

**Step 2: Run test to verify failure**

**Step 3: Add registrations** (same pattern as Task 2, in each processor's .cpp file)

```cpp
// face_enhancer
static domain::processor::ProcessorParamRegistrar face_enhancer_params_{
    .name = "face_enhancer",
    .params = {
        {"model", ParamType::String, "codeformer",
            {"codeformer", "gfpgan_1.2", "gfpgan_1.3", "gfpgan_1.4"}, "Enhancer model"},
        {"blend_factor", ParamType::Float, "0.8",
            {}, "Blend factor [0.0, 1.0]", std::make_pair(0.0, 1.0)},
        {"face_selector_mode", ParamType::String, "many",
            {"reference", "one", "many"}, "Face selection mode"},
        {"reference_face_path", ParamType::Path, "",
            {}, "Reference face image"},
    }
};

// expression_restorer
static domain::processor::ProcessorParamRegistrar expression_restorer_params_{
    .name = "expression_restorer",
    .params = {
        {"model", ParamType::String, "live_portrait",
            {"live_portrait"}, "Restorer model"},
        {"restore_factor", ParamType::Float, "0.8",
            {}, "Restore factor [0.0, 1.0]", std::make_pair(0.0, 1.0)},
        {"face_selector_mode", ParamType::String, "many",
            {"reference", "one", "many"}, "Face selection mode"},
        {"reference_face_path", ParamType::Path, "",
            {}, "Reference face image"},
    }
};

// frame_enhancer
static domain::processor::ProcessorParamRegistrar frame_enhancer_params_{
    .name = "frame_enhancer",
    .params = {
        {"model", ParamType::String, "real_esrgan_x4",
            {"real_esrgan_x2", "real_esrgan_x2_fp16", "real_esrgan_x4",
             "real_esrgan_x4_fp16", "real_esrgan_x8", "real_esrgan_x8_fp16",
             "real_hatgan_x4"}, "Frame enhancer model"},
        {"enhance_factor", ParamType::Float, "0.8",
            {}, "Enhance factor [0.0, 1.0]", std::make_pair(0.0, 1.0)},
    }
};
```

**Step 4: Run test to verify pass**

**Step 5: Commit**

```bash
git add -A
git commit -m "feat(processor): register parameter metadata for all processors"
```

---

## Task 4: Add CLI dynamic flag generation from registry

**Objective:** Generate CLI11 flags dynamically from ProcessorParamRegistry at startup.

**Files:**
- Modify: `src/app/cli/app_cli.cpp`
- Modify: `src/app/cli/app_cli.ixx`

**Step 1: Write integration test**

```cpp
// tests/integration/cli/cli_param_registration_test.cpp
#include <gtest/gtest.h>
#include <CLI/CLI.hpp>
#include <map>
#include <string>

import processor.param_registry;
import app.cli;

using namespace domain::processor;

class CliParamRegistrationTest : public ::testing::Test {};

TEST_F(CliParamRegistrationTest, HelpOutputContainsProcessorFlags) {
    // Capture CLI help output
    CLI::App app{"Test"};
    // ... register processor flags dynamically ...
    // Verify help contains --face-swapper-model etc.
    // This is more of a smoke test — real verification is in E2E.
    SUCCEED(); // Placeholder — actual implementation depends on CLI helper refactoring
}
```

**Step 2: Implement helper function in app_cli.cpp**

Add a helper function that registers processor params with CLI11:

```cpp
// In app_cli.cpp, add helper function:
// Returns map of {processor_name -> {param_name -> value}}
using ProcessorParamMap = std::map<std::string, std::map<std::string, std::string>>;

ProcessorParamMap register_processor_cli_params(
    CLI::App& app,
    const std::vector<std::string>& active_processors)
{
    using namespace domain::processor;
    ProcessorParamMap result;
    auto& registry = ProcessorParamRegistry::instance();

    for (const auto& proc_name : registry.all_processor_names()) {
        auto* meta = registry.find(proc_name);
        if (!meta) continue;

        for (const auto& param : meta->params) {
            // Build CLI flag name: face_swapper + model -> --face-swapper-model
            std::string cli_flag = "--";
            for (char c : proc_name) cli_flag += (c == '_' ? '-' : c);
            cli_flag += "-";
            for (char c : param.name) cli_flag += (c == '_' ? '-' : c);

            std::string* storage = &result[proc_name][param.name];

            switch (param.type) {
                case ParamType::Bool:
                    app.add_flag(cli_flag, *storage, param.description)
                        ->excludes("--task-config");
                    break;
                default: {
                    auto* opt = app.add_option(cli_flag, *storage, param.description);
                    opt->excludes("--task-config");
                    if (!param.allowed_values.empty()) {
                        opt->check(CLI::IsMember(param.allowed_values));
                    }
                    if (param.range) {
                        opt->check(CLI::Range(param.range->first, param.range->second));
                    }
                    break;
                }
            }
        }
    }
    return result;
}
```

**Step 3: Integrate into App::run()**

In the CLI setup section, after the existing `--processors` option:

```cpp
// After line 115 in app_cli.cpp (after --processors registration)
// Register all processor params dynamically
auto processor_params = register_processor_cli_params(app, {});
```

**Step 4: Build and verify compilation**

Run: `python build.py --action build`
Expected: Compilation success. Help output now shows processor flags.

**Step 5: Commit**

```bash
git add src/app/cli/app_cli.cpp src/app/cli/app_cli.ixx
git commit -m "feat(cli): add dynamic processor parameter flag generation from registry"
```

---

## Task 5: Enhance run_quick_mode to accept processor params

**Objective:** Pass parsed processor params into TaskConfig synthesis.

**Files:**
- Modify: `src/app/cli/app_cli.cpp`
- Modify: `src/app/cli/app_cli.ixx`

**Step 1: Modify run_quick_mode signature**

```cpp
// In app_cli.ixx, update declaration:
static int run_quick_mode(
    const std::vector<std::string>& source_paths,
    const std::vector<std::string>& target_paths,
    const std::string& output_path,
    const std::string& processors_str,
    const std::map<std::string, std::map<std::string, std::string>>& processor_params,
    const config::AppConfig& app_config);
```

**Step 2: Implement param injection in run_quick_mode**

In `run_quick_mode` (app_cli.cpp ~line 292), inject explicit params into each step:

```cpp
// After creating PipelineStep, inject user-specified params
for (const auto& proc : processors) {
    config::PipelineStep step;
    step.step = proc;
    step.enabled = true;

    // Inject user-specified CLI params
    auto it = processor_params.find(proc);
    if (it != processor_params.end()) {
        for (const auto& [param_name, param_value] : it->second) {
            if (!param_value.empty()) {
                step.params[param_name] = param_value;
            }
        }
    }

    task_config.pipeline.push_back(step);
}
```

Note: This requires `PipelineStep.params` to accept arbitrary key-value pairs in addition to the typed variant. The `StepParams` is currently a `std::variant<...>`, so we need a way to set params by name. Two options:
- Option A: Add a `std::map<std::string, std::string> extra_params` field to `PipelineStep`
- Option B: Parse string values into the variant at this point

**Recommendation: Option A** — add `extra_params` to PipelineStep and let the ConfigMerger/Validator handle conversion. This is simpler and doesn't require the CLI to know about variant internals.

```cpp
// In task_config.ixx, add to PipelineStep:
struct PipelineStep {
    std::string step;
    std::string name;
    bool enabled = true;
    StepParams params;
    std::map<std::string, std::string> cli_params; // New: explicit CLI overrides
};
```

**Step 3: Update the call site in App::run()**

```cpp
// Line 178 in app_cli.cpp — pass processor_params
exit_code = run_quick_mode(source_paths, target_paths, output_path, processors_str,
                            processor_params, *app_config);
```

**Step 4: Build and verify**

Run: `python build.py --action build`

**Step 5: Commit**

```bash
git add src/app/cli/app_cli.cpp src/app/cli/app_cli.ixx src/app/config/task_config.ixx
git commit -m "feat(cli): enhance quick mode to accept processor params from CLI"
```

---

## Task 6: Add --validate support for quick mode

**Objective:** Allow `--validate` to work with `-s/-t` quick mode parameters.

**Files:**
- Modify: `src/app/cli/app_cli.cpp`

**Step 1: Refactor validate logic**

Replace the current validate block (lines 168-174) with unified logic:

```cpp
if (validate_only) {
    config::TaskConfig task_config;
    bool has_config = false;

    if (!config_path.empty()) {
        // YAML mode: load from file
        auto loaded = config::load_task_config_from_file(config_path);
        if (loaded) {
            task_config = *loaded;
            has_config = true;
        } else {
            exit_code = 1;
        }
    } else if (!source_paths.empty() && !target_paths.empty()) {
        // Quick mode: synthesize from CLI params
        task_config = build_quick_task_config(source_paths, target_paths,
                                               output_path, processors_str,
                                               processor_params);
        has_config = true;
    } else {
        std::cerr << "Error: --validate requires --task-config or (-s, -t)\n";
        exit_code = 1;
    }

    if (has_config && exit_code == 0) {
        exit_code = run_validate(task_config, *app_config);
    }
}
```

Extract `run_validate(TaskConfig, AppConfig)` from the existing `run_validate(config_path, AppConfig)`.

**Step 2: E2E test**

Run quick mode with --validate:
```bash
cd build/bin/linux-x64-debug && ./FaceFusionCpp \
  -s /home/hui/workspace/projects/faceFusionCpp/assets/standard_face_test_images/lenna.bmp \
  -t /home/hui/workspace/projects/faceFusionCpp/assets/standard_face_test_images/woman.jpg \
  --processors face_swapper \
  --face-swapper-model inswapper_128_fp16 \
  --validate
```

Expected: Validation runs and reports OK.

**Step 3: Commit**

```bash
git add src/app/cli/app_cli.cpp
git commit -m "feat(cli): support --validate in quick mode with processor params"
```

---

## Task 7: E2E test with processor params

**Objective:** Verify the full pipeline works with CLI-specified processor params.

**Files:**
- Modify: `tests/e2e/configs/` (add new E2E test config or modify existing)

**Step 1: Manual E2E test**

```bash
cd build/bin/linux-x64-debug && ./FaceFusionCpp \
  -s ../../../assets/standard_face_test_images/lenna.bmp \
  -t ../../../assets/standard_face_test_images/woman.jpg \
  -o ./output/test/cli_enhancement/ \
  --processors face_swapper,face_enhancer \
  --face-swapper-model inswapper_128_fp16 \
  --face-swapper-face-selector-mode many \
  --face-enhancer-model codeformer \
  --face-enhancer-blend-factor 0.9
```

Expected: Processing completes successfully with the specified parameters.

**Step 2: Verify output exists and is valid**

Check output directory for processed image.

**Step 3: Run full test suite**

Run: `python build.py --action test --test-label unit && python build.py --action test --test-label integration`

**Step 4: Commit**

```bash
git add -A
git commit -m "test(e2e): verify CLI processor params in quick mode pipeline"
```

---

## Task 8: Final verification and cleanup

**Objective:** Ensure all tests pass and code quality is clean.

**Step 1: Run format checker**

Run: `python scripts/format_code.py`

**Step 2: Run pre-commit checks**

Run: `python scripts/pre_commit_check.py`

**Step 3: Run full test suite**

Run: `python build.py --action test --test-label unit`
Run: `python build.py --action test --test-label integration`

**Step 4: Final commit**

```bash
git add -A
git commit -m "chore: format and cleanup CLI enhancement implementation"
```
