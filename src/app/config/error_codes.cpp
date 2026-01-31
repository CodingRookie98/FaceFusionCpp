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
        {ErrorCode::E100_SystemError, "Generic system error"},
        {ErrorCode::E101_OutOfMemory, "System ran out of memory (RAM or VRAM)"},
        {ErrorCode::E102_DeviceNotFound, "CUDA device not found or lost"},
        {ErrorCode::E103_ThreadDeadlock, "Worker thread deadlock detected"},
        {ErrorCode::E104_GPUContextLost, "GPU context unexpectedly lost"},
        // Config
        {ErrorCode::E200_ConfigError, "Generic configuration error"},
        {ErrorCode::E201_YamlFormatInvalid, "YAML format is invalid"},
        {ErrorCode::E202_ParameterOutOfRange, "Parameter value is out of valid range"},
        {ErrorCode::E203_ConfigFileNotFound, "Configuration file not found"},
        {ErrorCode::E204_ConfigVersionMismatch, "Config version is incompatible"},
        {ErrorCode::E205_RequiredFieldMissing, "Required configuration field is missing"},
        {ErrorCode::E206_InvalidPath, "Path validation failed"},
        // Model
        {ErrorCode::E300_ModelError, "Generic model error"},
        {ErrorCode::E301_ModelLoadFailed, "Failed to load AI model"},
        {ErrorCode::E302_ModelFileMissing, "Model file does not exist"},
        {ErrorCode::E303_ModelChecksumMismatch, "Model file corrupted"},
        {ErrorCode::E304_ModelVersionIncompatible, "Model version not supported"},
        // Runtime
        {ErrorCode::E400_RuntimeError, "Generic runtime error"},
        {ErrorCode::E401_ImageDecodeFailed, "Failed to decode image"},
        {ErrorCode::E402_VideoOpenFailed, "Failed to open video file"},
        {ErrorCode::E403_NoFaceDetected, "No face detected in frame"},
        {ErrorCode::E404_FaceNotAligned, "Face alignment failed"},
        {ErrorCode::E405_ProcessorFailed, "Processor execution failed"},
        {ErrorCode::E406_OutputWriteFailed, "Failed to write output"},
        {ErrorCode::E407_TaskCancelled, "Task was cancelled"},
    };

    auto it = descriptions.find(code);
    return it != descriptions.end() ? it->second : "Unknown error";
}

std::string error_code_action(ErrorCode code) {
    static const std::unordered_map<ErrorCode, std::string> actions = {
        // System
        {ErrorCode::E101_OutOfMemory, "Reduce batch size or concurrent threads"},
        {ErrorCode::E102_DeviceNotFound, "Check GPU driver and hardware"},
        {ErrorCode::E103_ThreadDeadlock, "Restart the application"},
        // Config
        {ErrorCode::E201_YamlFormatInvalid, "Fix YAML syntax errors"},
        {ErrorCode::E202_ParameterOutOfRange, "Adjust parameter to valid range"},
        {ErrorCode::E203_ConfigFileNotFound, "Verify file path exists"},
        // Model
        {ErrorCode::E301_ModelLoadFailed, "Check model file integrity"},
        {ErrorCode::E302_ModelFileMissing, "Run model download script"},
        // Runtime
        {ErrorCode::E401_ImageDecodeFailed, "Skip corrupted frame"},
        {ErrorCode::E402_VideoOpenFailed, "Check video file format"},
        {ErrorCode::E403_NoFaceDetected, "Frame will be passed through"},
        {ErrorCode::E404_FaceNotAligned, "Frame will be skipped"},
    };

    auto it = actions.find(code);
    return it != actions.end() ? it->second : "Contact support";
}

} // namespace config
