# 模块参考 (Module Reference)

本文档提供了 FaceFusionCpp 中关键模块的快速参考，按 5 层架构组织。

## 1. 应用层 (`src/app/`)

| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `app.cli` | 应用程序入口和命令行接口。 | `App::run`, `App::run_pipeline` |
| `config` | 配置解析和验证。 | `AppConfig`, `TaskConfig`, `load_app_config` |

## 2. 服务层 (`src/services/`)

| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `services.pipeline` | 核心编排逻辑。 | `PipelineRunner`, `run_pipeline` |
| `services.shutdown` | 优雅停机处理。 | `ShutdownHandler::install` |

## 3. 领域层 (`src/domain/`)

### 3.1 人脸 (`domain.face`)
| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `model_registry` | 管理人脸检测/识别模型。 | `FaceModelRegistry::get_instance` |
| `processor_factory` | 创建处理器 (换脸、增强)。 | `create_processor` |

### 3.2 图像 (`domain.image`)
| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `image_utils` | 图像加载、保存和缩放。 | `load_image`, `save_image` |

### 3.3 AI (`domain.ai`)
| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `model_repository` | 管理磁盘上的物理模型文件。 | `ModelRepository::get_model_path` |

## 4. 平台层 (`src/platform/`)

| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `platform.fs` | 文件系统抽象。 | `file_exists`, `create_directory` |

## 5. 基础层 (`src/foundation/`)

### 5.1 AI (`foundation.ai`)
| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `inference_session` | ONNX Runtime 会话封装。 | `InferenceSession::run` |
| `session_pool` | 缓存推理会话以避免重载。 | `SessionPool::acquire` |

### 5.2 基础设施 (`foundation.infrastructure`)
| 模块 | 说明 | 关键接口 |
| :--- | :--- | :--- |
| `logger` | 日志系统 (spdlog 封装)。 | `Logger::info`, `Logger::error` |
| `error` | 标准化错误处理。 | `Result`, `Error` |
