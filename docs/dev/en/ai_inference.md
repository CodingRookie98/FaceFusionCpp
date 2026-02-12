# AI Inference Engine

FaceFusionCpp relies on **ONNX Runtime (ORT)** and **TensorRT** for high-performance AI inference. This document explains how we manage models and sessions.

## 1. Overview

The inference stack is encapsulated in the `Foundation` layer.

*   **Wrapper**: We do not use the ORT C++ API directly in domain code. Instead, we use `InferenceSession`.
*   **Provider Priority**:
    1.  **TensorRT**: Highest performance. Used if available and supported.
    2.  **CUDA**: Fallback for Nvidia GPUs if TensorRT fails or is disabled.
    3.  **CPU**: Last resort. Very slow.

## 2. Session Management

### 2.1 InferenceSession (`InferenceSession`)
A RAII wrapper around `Ort::Session`.
*   **Initialization**: Loads the model, configuring providers and session options (graph optimization level).
*   **Execution**: Handles input/output tensor binding and runs the `Run()` method.
*   **Error Handling**: Catches ORT exceptions and converts them to application errors.

### 2.2 SessionPool (`SessionPool`)
Creating an `InferenceSession` is expensive (especially with TensorRT, where it might trigger engine compilation).
*   **LRU Cache**: We cache recently used sessions.
*   **Key**: The model path (e.g., `assets/models/inswapper_128.onnx`).
*   **Policy**: If the pool is full, the least recently used session is evicted (unloaded from GPU).

## 3. Model Repository

The `ModelRepository` class manages the physical model files.

*   **Base Path**: Defined in `app_config.yaml` (`models.path`).
*   **Auto-Download**:
    *   If a requested model is missing, the repository can automatically download it from a configured URL (HuggingFace/GitHub).
    *   Validation: Checks file hash (SHA256) after download to ensure integrity.

## 4. TensorRT Optimization

TensorRT offers the best performance but requires an "Engine" file specific to the GPU model.

### 4.1 Engine Caching
*   **Mechanism**: ORT supports caching compiled TensorRT engines.
*   **Location**: `.cache/tensorrt/` (configurable).
*   **Filename**: Hashes the model signature + GPU properties.

### 4.2 First-Run Compilation
When you run a model for the first time with TensorRT:
1.  FaceFusionCpp loads the ONNX model.
2.  ORT/TensorRT compiles the graph (this takes **minutes**).
3.  The compiled engine is saved to disk.
4.  Subsequent runs load the cached engine instantly (< 1s).

> **Note**: Updating GPU drivers or the TensorRT version invalidates the cache, triggering recompilation.
