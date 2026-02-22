# 用户指南 (User Guide)

本指南详细介绍了 FaceFusionCpp 的核心功能以及如何高效使用它们进行图像和视频处理。

---

## 1. 核心功能 (Core Features)

FaceFusionCpp 基于模块化的流水线架构。核心处理单元称为 **Processor (处理器)**。您可以单独使用它们，也可以组合使用。

### 1.1 换脸处理器 (Face Swapper)

**它的作用是？** 这就是"换头术"的执行者。它负责把视频里原来的脸，替换成你找来的那张脸。

* **支持的模型**: `inswapper_128`, `inswapper_128_fp16`。(默认 `inswapper_128_fp16`)
* **小白该怎么选？**: 保持默认模型即可。关键在于选择**脸**的模式。
* **关键参数 (`face_selector_mode`)**:
  * `many` (默认值): 只要是个人脸，我都全换了。适合画面里只有一个人，或者你就是想恶搞换掉所有人的脸的场景。
  * `one`: 我只挑视频里脸最大（占比最大）的那个人换。
  * `reference`: 高级玩法。你除了传你要换的脸，还得上传一张照片告诉程序：“视频里如果长得跟这张照片像，才换，不像的旁边那个路人不换”。此时需要配套设置 `reference_face_path`。

### 1.2 人脸增强处理器 (Face Enhancer)

**它的作用是？** 换脸模型输出的五官是非常模糊的。这个模块像是"去马赛克"和"磨皮"，能极大地恢复五官的高清细节。**换脸必开！**

* **支持的模型**: `codeformer`, `gfpgan_1.2`, `gfpgan_1.3`, `gfpgan_1.4`。(默认 `gfpgan_1.4`)
* **小白该怎么选？**: 默认的 `gfpgan_1.4` 就很好，如果源视频极度老旧破损，可以尝试 `codeformer`。
* **关键参数 (`blend_factor`)**: 控制假脸的高清化程度，0 不增强，1 完全变成 3D 假人脸。(默认 `0.8`。这能保留大概两成的原始照片光影环境，最为自然)。

### 1.3 表情还原处理器 (Expression Restorer)

**它的作用是？** 消除死鱼眼！换出来的脸有时候眼球没有对准，或者表情僵硬。它负责让新换上去的脸，去努力模仿原视频人的眼球微表情。

* **支持的模型**: `live_portrait`。(默认 `live_portrait`)
* **小白该怎么选？**: 遇到眼神对不上的特写镜头，可以加这一步。
* **关键参数 (`restore_factor`)**: 还原比例，0 到 1 之间。(默认 `0.8`)。

### 1.4 全帧增强处理器 (Frame Enhancer)

**它的作用是？** 刚才的人脸增强只负责"脸"。如果原视频像马赛克一样，光是脸清晰了会极其突兀，它负责把整个画面（包含衣服背景）统统变成非常高清。

* **支持的模型**: `real_esrgan_x2`, `real_esrgan_x4_fp16` 等。
* **小白该怎么选？**: 非常吃显卡！只有在做非常精细的高要求图片时使用，普通配置不推荐拿来跑长视频。
* **关键参数 (`enhance_factor`)**: 增强强度，一般填默认值 `1.0` 即可。

---

## 2. 工作流 (Workflows)

### 2.1 图片处理 (Image Processing)
最简单的工作流。

**Linux (Bash)**:

```bash
./FaceFusionCpp -s source.jpg -t target.jpg -o output.png
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.png
```
*   **输入**: `-s` 与 `-t`均支持传入多个路径，以逗号分隔（或者在配置文件中配置列表）。
*   **输出**: `output.png`。格式由输出文件扩展名决定。

### 2.2 视频处理 (Video Processing)
应用程序自动处理视频输入。

**Linux (Bash)**:

```bash
./FaceFusionCpp -s source.jpg -t target.mp4 -o output.mp4
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.mp4 -o output.mp4
```
*   **音频**: 默认情况下，目标视频的音频会复制到输出视频中。
*   **性能**: 视频处理资源消耗较大。请参阅 [硬件指南](hardware_guide.md) 进行优化。

### 2.3 批量处理 (Batch Processing)
要处理多个文件或配置复杂的流水线，请使用配置文件。

1.  创建 `my_task.yaml` (参见 [配置指南](configuration_guide.md))。
2. 运行:

    **Linux (Bash)**:

    ```bash
    ./FaceFusionCpp -c my_task.yaml
    ```

    **Windows (PowerShell)**:

    ```powershell
    .\FaceFusionCpp.exe -c my_task.yaml
    ```

3. **目录输入**: 您可以在配置中的 `source_paths` 或 `target_paths` 指定目录路径。程序将处理该目录下所有有效的媒体文件。

---

## 3. 处理器组合 (Processor Combination)

FaceFusionCpp 的真正威力在于组合处理器。顺序很重要！

### 推荐流水线: 换脸 -> 增强 (绝大多数够用了)
这确保了换脸后的低分辨率脸得到高清处理，最符合实际使用预期。

> [!TIP]
> 可以在后面直接通过命令行的 `--processors` 拼接来快速执行。逗号不能有空格！

**命令行最快执行法**:
**Linux (Bash)**:

```bash
./FaceFusionCpp -s my_face.jpg -t target_video.mp4 -o result.mp4 --processors face_swapper,face_enhancer
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s my_face.jpg -t target_video.mp4 -o result.mp4 --processors face_swapper,face_enhancer
```

**或者用 YAML 配置文件**:
创建一个文件，随便叫比如 `do_job.yaml`：
```yaml
config_version: "1.0"
io:
  source_paths:
    - "my_face.jpg"
  target_paths:
    - "target_video.mp4"
  output:
    path: "result.mp4"

pipeline:
  - step: "face_swapper"
    name: "main_swap"
    params:
      model: "inswapper_128_fp16"
  - step: "face_enhancer"
    name: "post_enhancement"
    params:
      blend_factor: 0.8
```
然后执行它：

**Linux (Bash)**:
```bash
./FaceFusionCpp -c do_job.yaml
```

**Windows (PowerShell)**:
```powershell
.\FaceFusionCpp.exe -c do_job.yaml
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
