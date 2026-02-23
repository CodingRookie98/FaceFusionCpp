<div align="center">

# FaceFusionCpp

[![CI](https://github.com/CodingRookie98/faceFusionCpp/actions/workflows/ci.yml/badge.svg)](https://github.com/CodingRookie98/faceFusionCpp/actions/workflows/ci.yml)
[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()

> **High-Performance C++ Implementation of FaceFusion**
>
> 🚀 *Faster, Lighter, and Production-Ready*

[[中文文档]](README_CN.md) | [[Releases]](https://github.com/CodingRookie98/faceFusionCpp/releases) | [[Documentation]](docs/user/en/getting_started.md)

</div>

## 🌟 Introduction

**FaceFusionCpp** is a ground-up C++ rewrite of the popular open-source project [facefusion](https://github.com/facefusion/facefusion). It aims to deliver the exact same state-of-the-art face manipulation capabilities, but with exponentially better performance, a fraction of the memory footprint, and straightforward deployment.

## 🏆 Key Advantages (Why FaceFusionCpp?)

- 🚀 **Extreme Performance**: Engineered with a highly optimized C++ core and multi-threading architecture, achieving maximum throughput for both images and videos.
- 🧠 **Native Hardware Acceleration**: Deep, native integration with **NVIDIA CUDA** and **TensorRT** for lightning-fast AI inference without the overhead of Python wrappers.
- 📦 **Zero Environment Hell**: Say goodbye to complex Python virtual environments, dependency conflicts, and package version hell. Just download the pre-built binary and run.
- 💾 **Lower Memory Footprint**: Dramatically reduced RAM and VRAM consumption, allowing you to run larger batches or higher resolutions on modest hardware.
- 🌍 **Cross-Platform**: First-class support for both **Windows x64** and **Linux x64**, with seamless setup capabilities.

## ✨ Core Features

- 🎭 **Face Swapping**: Interchange faces in images and videos with staggering fidelity and structural integrity.
- ✨ **Face Enhancement**: Restore, sharpen, and upscale facial details using advanced restoration models.
- 😊 **Expression Restoration**: Recover and blend original facial expressions for incredibly natural-looking results.
- 🔍 **Super Resolution**: Upscale low-resolution inputs without sacrificing quality or introducing artifacts.
- ⚡ **Batch Processing**: Efficiently handle mixed batches of images and videos simultaneously.

## 📥 Installation Guide

The easiest way to get started is to download the latest pre-built binaries from our [Releases Page](https://github.com/CodingRookie98/faceFusionCpp/releases).

### System Requirements

To guarantee optimal performance and seamless execution, ensure your system meets these standards:

| Component | Requirement | Tested Version | Notes |
|-----------|-------------|----------------|-------|
| **OS** | Windows / Linux | Win10 / Ubuntu 24.04 | x64 architecture only |
| **CUDA** | ≥ 12 | 12.8.1 | Required for GPU acceleration |
| **cuDNN** | ≥ 9 | 9.19.0 | Required for GPU acceleration |
| **TensorRT**| ≥ 10 | 10.15.1 | Optional, for absolute max performance |

## 🚀 Quick Start

Extract the downloaded release archive, open a terminal in the folder, and run:

**Method 1: Command Line Arguments**
```bash
# Example CLI usage
./facefusion_cpp --source source.jpg --target target.mp4 --output result.mp4
```

**Method 2: Task Configuration File (Recommended for Complex Workflows)**
```bash
# Example using a YAML task configuration
./facefusion_cpp --task_config config/task_config.yaml
```

*For compilation instructions from source, please refer to the [Setup Guide](docs/dev/en/guides/setup.md).*

## 📖 Documentation

Explore our comprehensive documentation portal to unlock the full potential of FaceFusionCpp:

- **[Getting Started](docs/user/en/getting_started.md)**: Installation, system prerequisites, and your first run.
- **[User Guide](docs/user/en/user_guide.md)**: In-depth explanations of modules and workflows.
- **[CLI Reference](docs/user/en/cli_reference.md)**: Exhaustive command-line argument handbook.
- **[Configuration Guide](docs/user/en/configuration_guide.md)**: Advanced tuning via `app_config.yaml` and `task_config.yaml`.
- **[Hardware Guide](docs/user/en/hardware_guide.md)**: Hardware compatibility specifics and optimization strategies.
- **[FAQ](docs/user/en/faq.md)**: Solutions to common questions and troubleshooting.
- **[Developer Documentation](docs/dev/en/architecture/design.md)**: Architecture deep dives, processes, and contribution rules.

## 🙏 Acknowledgements

A massive thank you to the creators of the original Python [facefusion](https://github.com/facefusion/facefusion) project for their pioneering work in the open-source community. FaceFusionCpp builds upon their incredible foundation.

## ⚖️ Disclaimer

This software is strictly intended for **educational and research purposes only**. Users bear full responsibility for ensuring their usage complies with all applicable local, state, and federal laws and regulations. You must not use this software for illicit activities, deepfakes without consent, or to generate content that infringes upon the rights, privacy, or dignity of any individual.

The developers behind FaceFusionCpp unequivocally disclaim any liability or responsibility for the misuse of this tool.

## 📄 License

This project is licensed under the **GPLv3 License** - see the [LICENSE](LICENSE) file for complete details.

---

<p align="center">
  <sub>Built with ❤️ by the FaceFusionCpp Team. For the Community.</sub>
</p>
