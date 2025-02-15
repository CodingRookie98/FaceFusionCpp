Version 0.3.0

## Installation

If using GPU acceleration, please install the CUDA environment before running.

If using TensorRT acceleration, please install the TensorRT environment before running.

Version requirements:

>   **CUDA >= 12.2 (tested with CUDA 12.5)**

>   **CUDNN >= 9.2 (tested)**

>   **TensorRT >= 10.2 (tested) (if enabled)**

>   **ffmpeg >= 7.0.2 (tested)(for video)**

## Configuration File
### general

#### source_path

This configuration item is the source path, currently only supporting images. It supports single file paths as well as directory paths where multiple files are located.
>   [!NOTE]
>
>   This configuration item only takes effect for Face Swapper and Expression Restorer.

#### target_path
This configuration item is the target path, currently supporting both images and videos. It supports single file paths as well as directory paths where multiple images and/or videos are located.

#### reference_face_path

This configuration item is the path where the reference face image is located, currently only supporting single file path input.

#### output_path
This configuration item is the output path, supporting single file paths and directory paths. It is recommended to input a directory path.

### misc

#### force_download

Force to download all model files.
```
default: true
choice: false true
```

#### skip_download

Skip downloading model files.

```
default: false
choices: false true
```

#### log_level

Set the log level.

```
Default: info
choice: trace debug info warn error critical
```

### execution

#### execution_device_id

Specify the GPU device id for inference.

```
default: 0
```

#### execution_providers

Specify the providers for model inference.

```
default: cpu
choices: tensorrt cuda cpu
```

> Multiple providers can be specified simultaneously, with priority: tensorrt > cuda > cpu.

#### execution_thread_count

Specify the number of threads for inference tasks. Reducing this can reduce memory and GPU memory usage.

```
default: 1
```

### tensorrt

#### enable_engine_cache
Set whether to enable TensorRT engine caching. Enabling this can avoid long engine builds every time TensorRT is enabled. After enabling this, the engine only needs to be built once, and there is no need to build it again.
```
default: true
choices: false true
```

#### enable_embed_engine
Set whether to enable the embedded engine.
```
default: true
choices: false true
```
>   [!TIP]
>
>   It is recommended to enable this option when using TensorRT.

#### trt_max_workspace_size
Set the trt_max_workspace_size in GB.
```
default: none
example: 2.5
```

### memory

#### Processor Memory Strategy

**Strict**: Processors are created only when needed and are immediately destroyed after use.
>   [!TIP]
>
>   This mode can significantly reduce GPU memory and RAM usage during runtime. However, since each processor creation involves loading model data, processing speed may be slower.

**Tolerant**: All user-specified processors are created at program start and only destroyed when the program ends.
````
example: Strict
````


### face_analyser

#### face_detector_model
Set the face detection model.

```
Default: yoloface
Choices: many retinaface scrfd yoloface
```

#### face_detector_size
Set the size of the face detector.
```
Default: 640x640
Choices: 160x160 320x320 480x480 512x512 640x640
```

#### face_detector_score
Set the confidence of the face detector.
```
Default: 0.5
Range: 0 to 1 at 0.05
Example: 0.7
```

#### face_landmarker_score
Set the confidence of the face landmarker.
```
Argument: --face-landmarker-score
Default: 0.5
Range: 0 to 1 at 0.05
Example: 0.7
```

### face_selector

#### face_selector_mode
Set the face selector mode.
```
Default: many
Choices: many one reference
Example: one
```

#### face_selector_order
Set the order of the face selector.
```
Default: left-right
Choices: left-right right-left top-bottom bottom-top small-large large-small best-worst worst-best
Example: best-worst
```

#### face_selector_age_start
Set the starting age value of the face selector.
```
example: 10
```

#### face_selector_age_end
Set the ending age value of the face selector.
```
example: 80
```

#### face_selector_gender
Set the gender of the face selector.
```
Default: None
Choices: male female
Example: male
```

#### reference_face_position
Set the position of the reference face in the image.
```
Default: 0
Example: 1
```
> It is recommended to align or crop the reference face image to only contain one face in the image.

#### reference_face_distance
Set the similarity (distance) between the reference face and the target face.
```
Default: 0.6
Range: 0 to 1.5 at 0.05
Example: 0.8
```

### face_mask

#### face_mask_types
Set the types of face masks.
```
Default: box
Choices: box occlusion region
Example: box occlusion
```

#### face_mask_blur
Set the blur level of the face mask.
```
Default: 0.3
Range: 0 to 1 at 0.05
Example: 0.6
```

#### face_mask_padding
Set the face padding boundaries, default order is top, right, bottom, left.
```
Default: 0 0 0 0
Example: 1 2
```

#### face_mask_regions
Set the facial regions used for face masks.
```
Default: all
Choices: skin left-eyebrow right-eyebrow left-eye right-eye glasses nose mouth upper-lip lower-lip
Example: left-eye right-eye eye-glasses
```

### image

#### output_image_quality
Set the output image quality.
```
Default: 100
Range: 0 to 100 at 1
Example: 60
```

#### output_image_resolution
Set the output image resolution.
```
Default: None
Example: 1920x1080
```

### video

#### video_segment_duration
Enable video segment processing with a duration (seconds) for each segment.
```
default: 0
example: 30
```
>   [!TIP]
>
> It is recommended to set a value when processing long videos, which can effectively reduce disk usage.

#### output_video_encoder

Specify the video encoder.
```
Default: libx264
Choices: libx264 libx265 libvpx-vp9 h264_nvenc hevc_nvenc h264_amf hevc_amf
Example: libx265
```

#### output_video_preset
Set the video encoder preset.
```
Default: veryfast
Choices: ultrafast superfast veryfast faster fast medium slow slower veryslow
Example: faster
```

#### output_video_quality
Set the output video quality.
```
Default: 80
Range: 0 to 100 at 1
Example: 60
```

#### output_audio_encoder
Set the audio encoder.
```
Default: aac
Choices: aac libmp3lame libopus libvorbis
Example: libmp3lame
```

#### skip_audio
Set whether to skip audio processing.
```
Default: false
choice: true false
Example: true
```

#### temp_frame_format
Set the format of the extracted video frames.
```
default: png
choice: png jpg bmp
```

### frame_processors

#### frame_processors
Set the processor sequence.
```
Default: face_swapper
Choices: face_enhancer face_swapper expression_restorer frame_enhancer
Example 1: face_swapper face_enhancer
Example 2: face_enhancer face_swapper expression_restorer face_enhancer
```

#### face_enhancer_model
Set the model for face enhancer.
```
Default: gfpgan_1.4
Choices: codeformer gfpgan_1.2 gfpgan_1.3 gfpgan_1.4
Example: codeformer
```

#### face_enhancer_blend
Set the blending level for face enhancement.
```
Default: 80
Range: 0 to 100 at 1
Example: 60
```

#### face_swapper_model

Set the model for face swapper.
```
Default: inswapper_128_fp16
Choices: inswapper_128 inswapper_128_fp16
Example: inswapper_128
```

#### expression_restorer_model

Set the model for expression_restorer.

```
Default: liveportrait
Choices: liveportrait
````

#### expression_restorer_factor

Set the intensity of expression_restorer.

```
Default: 80
Range: 0 to 100 at 1
Example: 70
```

#### frame_enhancer_model

Set the model for frame_enhancer.

```
Default: real_hatgan_x4
Choice: real_hatgan_x4 real_esrgan_x2 real_esrgan_x2_fp16 real_esrgan_x4 real_esrgan_x4_fp16 real_esrgan_x8 real_esrgan_x8_fp16
Example: real_esrgan_x8_fp16
```

#### frame_enhancer_blend

Set the intensity of frame_enhancer.

```
Default: 80
Range: 0 to 100 at 1
Example: 70
```