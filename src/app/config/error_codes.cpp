/**
 * @file error_codes.cpp
 * @brief Error code string conversion implementations
 */
module;

#include <string>
#include <unordered_map>

module config.types;

namespace config {

std::string error_code_description(ErrorCode code) {
    static const std::unordered_map<ErrorCode, std::string> descriptions = {
        // System
        {ErrorCode::E100SystemError, "Generic system error"},
        {ErrorCode::E101OutOfMemory, "System ran out of memory (RAM or VRAM)"},
        {ErrorCode::E102DeviceNotFound, "CUDA device not found or lost"},
        {ErrorCode::E103ThreadDeadlock, "Worker thread deadlock detected"},
        {ErrorCode::E104GpuContextLost, "GPU context unexpectedly lost"},
        // Config
        {ErrorCode::E200ConfigError, "Generic configuration error"},
        {ErrorCode::E201YamlFormatInvalid, "YAML format is invalid"},
        {ErrorCode::E202ParameterOutOfRange, "Parameter value is out of valid range"},
        {ErrorCode::E203ConfigFileNotFound, "Configuration file not found"},
        {ErrorCode::E204ConfigVersionMismatch, "Config version is incompatible"},
        {ErrorCode::E205RequiredFieldMissing, "Required configuration field is missing"},
        {ErrorCode::E206InvalidPath, "Path validation failed"},
        // Model
        {ErrorCode::E300ModelError, "Generic model error"},
        {ErrorCode::E301ModelLoadFailed, "Failed to load AI model"},
        {ErrorCode::E302ModelFileMissing, "Model file does not exist"},
        {ErrorCode::E303ModelChecksumMismatch, "Model file corrupted"},
        {ErrorCode::E304ModelVersionIncompatible, "Model version not supported"},
        // Runtime
        {ErrorCode::E400RuntimeError, "Generic runtime error"},
        {ErrorCode::E401ImageDecodeFailed, "Failed to decode image"},
        {ErrorCode::E402VideoOpenFailed, "Failed to open video file"},
        {ErrorCode::E403NoFaceDetected, "No face detected in frame"},
        {ErrorCode::E404FaceNotAligned, "Face alignment failed"},
        {ErrorCode::E405ProcessorFailed, "Processor execution failed"},
        {ErrorCode::E406OutputWriteFailed, "Failed to write output"},
        {ErrorCode::E407TaskCancelled, "Task was cancelled"},
    };

    auto it = descriptions.find(code);
    return it != descriptions.end() ? it->second : "Unknown error";
}

std::string error_code_action(ErrorCode code) {
    static const std::unordered_map<ErrorCode, std::string> actions = {
        // System
        {ErrorCode::E101OutOfMemory, "Reduce batch size or concurrent threads"},
        {ErrorCode::E102DeviceNotFound, "Check GPU driver and hardware"},
        {ErrorCode::E103ThreadDeadlock, "Restart the application"},
        // Config
        {ErrorCode::E201YamlFormatInvalid, "Fix YAML syntax errors"},
        {ErrorCode::E202ParameterOutOfRange, "Adjust parameter to valid range"},
        {ErrorCode::E203ConfigFileNotFound, "Verify file path exists"},
        // Model
        {ErrorCode::E301ModelLoadFailed, "Check model file integrity"},
        {ErrorCode::E302ModelFileMissing, "Run model download script"},
        // Runtime
        {ErrorCode::E401ImageDecodeFailed, "Skip corrupted frame"},
        {ErrorCode::E402VideoOpenFailed, "Check video file format"},
        {ErrorCode::E403NoFaceDetected, "Frame will be passed through"},
        {ErrorCode::E404FaceNotAligned, "Frame will be skipped"},
    };

    auto it = actions.find(code);
    return it != actions.end() ? it->second : "Contact support";
}

} // namespace config
