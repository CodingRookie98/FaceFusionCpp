# Module Reference

This document provides a quick reference to the key modules in FaceFusionCpp, organized by the 5-layer architecture.

## 1. Application Layer (`src/app/`)

| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `app.cli` | Application entry point and command-line interface. | `App::run`, `App::run_pipeline` |
| `config` | Configuration parsing and validation. | `AppConfig`, `TaskConfig`, `load_app_config` |

## 2. Services Layer (`src/services/`)

| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `services.pipeline` | Core orchestration logic. | `PipelineRunner`, `run_pipeline` |
| `services.shutdown` | Graceful shutdown handling. | `ShutdownHandler::install` |

## 3. Domain Layer (`src/domain/`)

### 3.1 Face (`domain.face`)
| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `model_registry` | Manages face detection/recognition models. | `FaceModelRegistry::get_instance` |
| `processor_factory` | Creates processors (swapper, enhancer). | `create_processor` |

### 3.2 Image (`domain.image`)
| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `image_utils` | Image loading, saving, and resizing. | `load_image`, `save_image` |

### 3.3 AI (`domain.ai`)
| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `model_repository` | Manages physical model files on disk. | `ModelRepository::get_model_path` |

## 4. Platform Layer (`src/platform/`)

| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `platform.fs` | File system abstraction. | `file_exists`, `create_directory` |

## 5. Foundation Layer (`src/foundation/`)

### 5.1 AI (`foundation.ai`)
| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `inference_session` | Wrapper for ONNX Runtime session. | `InferenceSession::run` |
| `session_pool` | Caches inference sessions to avoid reload. | `SessionPool::acquire` |

### 5.2 Infrastructure (`foundation.infrastructure`)
| Module | Description | Key Interfaces |
| :--- | :--- | :--- |
| `logger` | Logging system (spdlog wrapper). | `Logger::info`, `Logger::error` |
| `error` | Standardized error handling. | `Result`, `Error` |
