# 用户指南 (User Guide)

本指南详细介绍了 FaceFusionCpp 的核心功能以及如何高效使用它们进行图像和视频处理。

---

## 1. 核心功能 (Core Features)

FaceFusionCpp 基于模块化的流水线架构。核心处理单元称为 **Processor (处理器)**。您可以单独使用它们，也可以组合使用。

### 1.1 换脸处理器 (Face Swapper)
这是应用程序的核心功能。它检测目标图像/视频中的人脸，并将其替换为源图像中的人脸。
*   **支持的模型**: `inswapper_128`, `inswapper_128_fp16`。
*   **关键参数**: `face_selector_mode` (人脸选择模式)
    *   `many` (默认): 替换所有检测到的人脸。
    *   `one`: 仅替换最大的一张人脸。
    *   `reference`: 仅替换与 `reference_face_path` 指定图片相似的人脸。

### 1.2 人脸增强处理器 (Face Enhancer)
使用 GFPGAN/CodeFormer 恢复人脸的细节和清晰度。这是换脸后的强烈推荐步骤，因为换脸模型的输出通常只有 128x128 分辨率。
*   **支持的模型**: `codeformer`, `gfpgan_1.2`, `gfpgan_1.3`, `gfpgan_1.4`。
*   **关键参数**: `blend_factor` (0.0 - 1.0)。控制增强后的人脸与原始人脸的混合程度。

### 1.3 表情还原处理器 (Expression Restorer)
使用 LivePortrait 恢复换脸后的人脸表情神态，使其更加生动贴合原始照片或视频。
*   **支持的模型**: `live_portrait`。
*   **关键参数**: `restore_factor` (0.0 - 1.0)。控制表情还原的比例。

### 1.4 全帧增强处理器 (Frame Enhancer)
使用 Real-ESRGAN 对整张图片或视频帧进行超分辨率放大。用于提升低分辨率目标的画质。
*   **支持的模型**: `real_esrgan_x2`, `real_esrgan_x2_fp16`, `real_esrgan_x4`, `real_esrgan_x4_fp16`, `real_esrgan_x8`, `real_esrgan_x8_fp16`, `real_hatgan_x4`。
*   **关键参数**: `enhance_factor` (增强强度)。

---

## 2. 工作流 (Workflows)

### 2.1 图片处理 (Image Processing)
最简单的工作流。
```powershell
FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.png
```
*   **输入**: `-s` 与 `-t`均支持传入多个路径，以逗号分隔（或者在配置文件中配置列表）。
*   **输出**: `output.png`。格式由输出文件扩展名决定。

### 2.2 视频处理 (Video Processing)
应用程序自动处理视频输入。
```powershell
FaceFusionCpp.exe -s source.jpg -t target.mp4 -o output.mp4
```
*   **音频**: 默认情况下，目标视频的音频会复制到输出视频中。
*   **性能**: 视频处理资源消耗较大。请参阅 [硬件指南](hardware_guide.md) 进行优化。

### 2.3 批量处理 (Batch Processing)
要处理多个文件或配置复杂的流水线，请使用配置文件。

1.  创建 `my_task.yaml` (参见 [配置指南](configuration_guide.md))。
2.  运行:
    ```powershell
    FaceFusionCpp.exe -c my_task.yaml
    ```
3.  **目录输入**: 您可以在配置中的 `source_paths` 或 `target_paths` 指定目录路径。程序将处理该目录下所有有效的媒体文件。

---

## 3. 处理器组合 (Processor Combination)

FaceFusionCpp 的真正威力在于组合处理器。顺序很重要！

### 推荐流水线: 换脸 -> 增强
这确保了换脸后的低分辨率人脸在最终输出前被增强。

**命令行**:
```powershell
FaceFusionCpp.exe ... --processors face_swapper,face_enhancer
```

**YAML 配置**:
```yaml
pipeline:
  - step: "face_swapper"
    name: "main_swap"
    params:
      model: "inswapper_128_fp16"
  - step: "face_enhancer"
    name: "post_enhancement"
    params:
      blend_factor: 1.0
```

### 高级流水线: 换脸 -> 表情还原 -> 增强 -> 放大
1.  **Swap**: 替换人脸。
2.  **Expression Restore**: 恢复人脸神态。
3.  **Enhance Face**: 修复人脸细节。
4.  **Enhance Frame**: 放大整张图像 (例如 2倍或 4倍)。

```yaml
pipeline:
  - step: "face_swapper"
  - step: "expression_restorer"
    params:
      model: "live_portrait"
  - step: "face_enhancer"
  - step: "frame_enhancer"
    params:
      model: "real_esrgan_x2_fp16"
```
