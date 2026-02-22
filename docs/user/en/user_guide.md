# User Guide

This guide details the core features of FaceFusionCpp and how to effectively use them for image and video processing.

---

## 1. Core Features

FaceFusionCpp is built upon a modular pipeline architecture. The core processing units are called **Processors**. You can use them individually or combine them.

### 1.1 Face Swapper

**What does it do?** This is the executor of the "head transplant." It replaces the original face in the video with the face you provide.

* **Supported Models**: `inswapper_128`, `inswapper_128_fp16`. (Default `inswapper_128_fp16`)
* **Beginner Tip**: Stick to the default model. The key is picking the right **face mode**.
* **Crucial Parameters (`face_selector_mode`)**:
  * `many` (Default): Swap every single face detected on screen. Ideal for solo shots, or if you just want to spoof everyone's face.
  * `one`: Only swaps the **largest** face (the protagonist), ignoring background actors.
  * `reference`: Advanced mode. Alongside the target face to swap *in*, you must upload an original photo telling the program "Only swap faces that look like *this* person, leave bystanders alone." Requires setting `reference_face_path`.

### 1.2 Face Enhancer

**What does it do?** The face swapping model outputs extremely blurry facial features. This module acts as an "anti-aliasing and skin softening" filter that vastly restores crisp high-definition details back into the facial features. **A must for swapping!**

* **Supported Models**: `codeformer`, `gfpgan_1.2`, `gfpgan_1.3`, `gfpgan_1.4`. (Default `gfpgan_1.4`)
* **Beginner Tip**: The default `gfpgan_1.4` is perfect. If the source video is extremely old or torn, you can try `codeformer`.
* **Crucial Parameters (`blend_factor`)**: Controls the degree of high-definition upscaling on the false face (0.0 - 1.0). (Default `0.8`. This retains roughly 20% of the original photo's ambient lighting environment, keeping it looking highly natural.)

### 1.3 Expression Restorer

**What does it do?** Eliminates dead fish eyes! The swapped face sometimes has misaligned eyeballs or stiff micro-expressions. This step forces the newly swapped face to try and mimic the minute eye and mouth movements of the original person in the video.

* **Supported Models**: `live_portrait`. (Default `live_portrait`)
* **Beginner Tip**: You can add this step whenever you notice mismatched gazes in close-up shots.
* **Crucial Parameters (`restore_factor`)**: Restoration blend ratio (0.0 - 1.0). (Default `0.8`).

### 1.4 Frame Enhancer

**What does it do?** The previously mentioned face enhancer only touches the "face". If the original video is blurry, a clean face will look extremely out of place. This module turns the entire frame (including clothes and background) into high definition.

* **Supported Models**: `real_esrgan_x2`, `real_esrgan_x4_fp16`, etc.
* **Beginner Tip**: Extremely heavy on the graphics card! Only recommended for high-demand still images. Beginners are advised not to use this on long videos to avoid VRAM blowouts.
* **Crucial Parameters (`enhance_factor`)**: Unsharp mask intensity, generally fine to leave at the default `1.0`.

---

## 2. Workflows

### 2.1 Image Processing

The simplest workflow imaginable.

**Linux (Bash)**:
```bash
./FaceFusionCpp -s source.jpg -t target.jpg -o output.png
```

**Windows (PowerShell)**:
```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.png
```

* **Input**: Both `-s` and `-t` support passing multiple paths separated by commas (or via a list inside a config file).
* **Output**: `output.png`. The format is determined by the output file extension.

### 2.2 Video Processing

The application will automatically multiplex video inputs.

**Linux (Bash)**:
```bash
./FaceFusionCpp -s source.jpg -t target.mp4 -o output.mp4
```

**Windows (PowerShell)**:
```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.mp4 -o output.mp4
```

* **Audio**: By default, the target video's audio track is perfectly copied over to the output video.
* **Performance**: Video processing consumes immense resources. Please refer to our `hardware_guide.md` for optimization.

### 2.3 Batch Processing

For processing multiple files or configuring complex pipelines, utilize configuration files.

1. Create `my_task.yaml` (See the `configuration_guide.md`).
2. Run:

    **Linux (Bash)**:
    ```bash
    ./FaceFusionCpp -c my_task.yaml
    ```

    **Windows (PowerShell)**:
    ```powershell
    .\FaceFusionCpp.exe -c my_task.yaml
    ```

3. **Directory Inputs**: You can map directories inside `source_paths` or `target_paths` in the config. The program sweeps the folder and processes every valid media file.

---

## 3. Processor Combination

FaceFusionCpp's true power lies within its pipeline processing architecture. Order matters heavily!

### Recommended Pipeline: Swap -> Enhance (Matches 99% of Needs)

This ensures the low-resolution face output from the swapper is passed into the sharpener before final output.

> [!TIP]
> You can quickly execute this via the `--processors` command line flag. Do not put spaces between the commas!

**The Quick CLI Method**:

**Linux (Bash)**:
```bash
./FaceFusionCpp -s my_face.jpg -t target_video.mp4 -o result.mp4 --processors face_swapper,face_enhancer
```

**Windows (PowerShell)**:
```powershell
.\FaceFusionCpp.exe -s my_face.jpg -t target_video.mp4 -o result.mp4 --processors face_swapper,face_enhancer
```

**Or via YAML Configuration**:

Create a file, ex. `do_job.yaml`:

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

And then run it:

**Linux (Bash)**:
```bash
./FaceFusionCpp -c do_job.yaml
```

**Windows (PowerShell)**:
```powershell
.\FaceFusionCpp.exe -c do_job.yaml
```

### Advanced Pipeline: Swap -> Restorer -> Face Enhance -> Frame Enhance

1. **Swap**: Replaces the face.
2. **Expression Restore**: Matches the subtle micro-expressions.
3. **Enhance Face**: Restores local face details.
4. **Enhance Frame**: Upscales the entire image (e.g., 2x or 4x).

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
