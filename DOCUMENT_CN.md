Version 0.3.0

## Installation

如果使用GPU加速处理，请在运行前安装CUDA环境。

如果使用TensorRT加速处理，请在运行前安装TensorRT环境。

 版本要求:

>   **CUDA >= 12.2 (tested with CUDA 12.5)**

>   **CUDNN >= 9.2 (tested)**

>   **TensorRT >= 10.2 (tested) (if enabled)**
>
>   **ffmpeg >= 7.0.2 (tested)(for video)**



## Configuration File
### general

#### source_path

此配置项为源路径，目前只支持图片。支持单文件路径以及多个文件所在的目录路径。
>   [!NOTE]
>
>   此配置项只对Face Swapper及Expression Restorer生效。

#### target_path
此配置项为目标路径，目前支持图片和视频。支持单文件路径以及多个图片和（或）视频文件所在的目录路径。

#### reference_face_path

此配置项为参考脸部的图片所在的路径，目前只支持单文件路径输入。

#### output_path
此配置项为输出路径，支持单文件路径及目录路径输入。推荐输入目录路径。



### misc

#### force_download

强制下载所有模型文件。
```
default: true
choice: false true
```

#### skip_download

跳过模型文件下载。

```
default: false
choices: false true
```

#### log_level

设置日志级别。

```
Default: info
choice: trace debug info warn error critical
```



### execution

#### execution_device_id

指定进行推理的GPU设备id.

```
default: 0
```

#### execution_providers

指定模型推理时的提供者。

```
default: cpu
choices: tensorrt cuda cpu
```

> 多个供应者可以同时指定tensorrt > cuda > cpu。

#### execution_thread_count

指定进行推理任务的线程数量。减小此项可降低内存和显存占用。

```
default: 1
```



### tensorrt

#### enable_engine_cache
设置TensorRT引擎是否启用缓存。启用此项可以避免每次启用TensorRT时都进行长时间的引擎构建，启用此项后只需要构建一次，之后便不需要再次构建。
```
default: true
choices: false true
```

#### enable_embed_engine
设置是否开启embed engine。
```
default: true
choices: false true
```
>   [!TIP]
>
>   It is recommended to enable this option when using TensorRT.

#### trt_max_workspace_size
设置trt_max_workspace_size，单位为GB。
```
default: none
example: 2.5
```



### memory

#### processor_memory_strategy
strict: 需要用到时才创建，用完则立即销毁。
>   [!TIP]
>
>   此模式可以在运行时大幅降低显存及内存占用，因为每次创建processor时会花费时间读取模型数据，所以会将降低处理速度。

tolerant：进入程序便创建所有用户指定的processor，程序结束时才销毁。
````
example: Strict
````


### face_analyser

#### face_detector_model
设置脸部检测模型。

```
Default: yoloface
Choices: many retinaface scrfd yoloface
```

#### face_detector_size
设置face detector的大小。
```
Default: 640x640
Choices: 160x160 320x320 480x480 512x512 640x640
```

#### face_detector_score
设置face detector的置信度。
```
Default: 0.5
Range: 0 to 1 at 0.05
Example: 0.7
```

#### face_landmarker_model
设置face landmarker的模型。
```
Default: 2dfan4
Choices: many 2dfan4 peppa_wutz
```

#### face_landmarker_score
设置face landmarker的置信度。
```
Argument: --face-landmarker-score
Default: 0.5
Range: 0 to 1 at 0.05
Example: 0.7
```



### face_selector

#### face_selector_mode
设置face selector模式。
```
Default: many
Choices: many one reference
Example: one
```

####  face_selector_order
设置face selector的顺序。
```
Default: left-right
Choices: left-right right-left top-bottom bottom-top small-large large-small best-worst worst-best
Example: best-worst
```

#### ace_selector_age_start
设置face selector的年龄起始值。
```
example: 10
```

#### ace_selector_age_end
设置face selector的年龄结束值。
```
example: 80
```

#### face_selector_gender
设置face selector的性别。
```
Default: None
Choices: male female
Example: male
```

#### reference_face_position
设置参考的脸部在图片中的位置。
```
Default: 0
Example: 1
```
> 推荐将参考的脸部图片进行脸部对齐或者裁剪到图片中仅有一张脸。

#### reference_face_distance
设置参考脸和目标脸的相似度（distance）。
```
Default: 0.6
Range: 0 to 1.5 at 0.05
Example: 0.8
```



### face_masker

#### face_occluder_model
选择负责脸部遮罩的模型。
```
Default: xseg_1
Choices: xseg_1 xseg_2
Example: xseg_1
```

#### face_parser_model 
选择负责脸部区域蒙版的模型。
```
Default: bisenet_resnet_34
Choices: bisenet_resnet_18 bisenet_resnet_34
Example: bisenet_resnet_34
```

#### face_mask_types
设置脸部遮罩类型。
```
Default: box
Choices: box occlusion region
Example: box occlusion
```

#### face_mask_blur
设置脸部遮罩模糊度。
```
Default: 0.3
Range: 0 to 1 at 0.05
Example: 0.6
```

#### face_mask_padding
设置面部填充边界，默认顺序为top, right, bottom, left
```
Default: 0 0 0 0
Example: 1 2
```

#### face_mask_regions
设置用于脸部遮罩的面部区域。
```
Default: all
Choices: skin left-eyebrow right-eyebrow left-eye right-eye glasses nose mouth upper-lip lower-lip
Example: left-eye right-eye eye-glasses
```



### image

#### output_image_quality
设置图片输出质量。
```
Default: 100
Range: 0 to 100 at 1
Example: 60
```

#### output_image_resolution
设置图片输出分辨略。
```
Default: None
Example: 1920x1080
```



### video

#### video_segment_duration
启用视频分段的处理的片段时长（秒）。
```
default: 0
example: 30
```
>   [!TIP]
>
> 当处理长视频时推荐设置一个值，可以有效减少磁盘占用。

#### output_video_encoder

指定视频编码器。
```
Default: libx264
Choices: libx264 libx265 libvpx-vp9 h264_nvenc hevc_nvenc h264_amf hevc_amf
Example: libx265
```

#### output_video_preset
设置视频编码器预设。
```
Default: veryfast
Choices: ultrafast superfast veryfast faster fast medium slow slower veryslow
Example: faster
```

#### output_video_quality
设置输出的视频质量。
```
Default: 80
Range: 0 to 100 at 1
Example: 60
```

#### output_audio_encoder
设置音频编码器。
```
Default: aac
Choices: aac libmp3lame libopus libvorbis
Example: libmp3lame
```

#### skip_audio
设置是否跳过音频处理。
```
Default: false
choice: true false
Example: true
```

#### temp_frame_format
设置提取的视频帧格式。
```
default: png
choice: png jpg bmp
```



### frame_processors

#### frame_processors
设置处理器序列。
```
Default: face_swapper
Choices: face_enhancer face_swapper expression_restorer frame_enhancer
Example 1: face_swapper face_enhancer
Example 2: face_enhancer face_swapper expression_restorer face_enhancer
```

#### face_enhancer_model
设置face enhancer的模型。
```
Default: gfpgan_1.4
Choices: codeformer gfpgan_1.2 gfpgan_1.3 gfpgan_1.4
Example: codeformer
```

#### face_enhancer_blend
设置脸部增强的混合度。
```
Default: 80
Range: 0 to 100 at 1
Example: 60
```

#### face_swapper_model

设置face swapper的模型。
```
Default: inswapper_128_fp16
Choices: inswapper_128 inswapper_128_fp16
Example: inswapper_128
```

#### expression_restorer_model

设置expression_restorer的模型。

```
Default: liveportrait
Choices: liveportrait
Example: liveportrait
```

#### expression_restorer_factor

设置expression_restorer的强度。

```
Default: 80
Range: 0 to 100 at 1
Example: 70
```

#### frame_enhancer_model

设置frame_enhancer的模型。

```
Default: real_hatgan_x4
Choice: real_hatgan_x4 real_esrgan_x2 real_esrgan_x2_fp16 real_esrgan_x4 real_esrgan_x4_fp16 real_esrgan_x8 real_esrgan_x8_fp16
Example: real_esrgan_x8_fp16
```

#### frame_enhancer_blend

设置frame_enhancer的强度。

```
Default: 80
Range: 0 to 100 at 1
Example: 70
```

