module;
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <onnx/onnx_pb.h>
#include <onnxruntime_cxx_api.h>
#include <format>

module domain.face.swapper;

import :impl_base;
import :types;
import :inswapper;
import :mask_compositor;
import domain.face.helper;
import domain.face.masker;
import foundation.ai.inference_session;

namespace domain::face::swapper {

using namespace domain::face::types;
using namespace domain::face::helper;

InSwapper::InSwapper() : FaceSwapperImplBase() {}

void InSwapper::load_model(const std::string& model_path,
                           const foundation::ai::inference_session::Options& options) {
    FaceSwapperImplBase::load_model(model_path, options);
    m_initializer_array.clear();
    init();

    // Load Maskers (TODO: Path should be configurable or standard resource path)
    // For now, assume standard models exist in "models" directory relative to binary or hardcoded
    // for now? In legacy code, they were loaded via Hub. Let's assume user provides model paths via
    // some config, but here interface only takes swapper model path. We will lazy load or use
    // hardcoded paths relative to the swapper model path or a known location.

    // TEMPORARY: Hardcoded paths for Phase 2 proof of concept.
    // Ideally this should be passed in Options or via a Resource Manager.
    try {
        // m_occluder = domain::face::masker::create_occlusion_masker("models/face_occluder.onnx",
        // options); m_region_masker =
        // domain::face::masker::create_region_masker("models/face_parser.onnx", options); Commented
        // out to avoid crash if files don't exist. We will initialize them only if they are
        // requested in swap options? No, swap options are per-frame. Models must be preloaded.

        // Let's check if models exist next to the swapper model?
        // std::filesystem::path base_path = std::filesystem::path(model_path).parent_path();
        // ...

        // For now, keep them null. We need a mechanism to load them.
        // Maybe add `load_masker_models(occluder_path, region_path)` to IFaceSwapper?
        // Or just lazy load if we had paths.
    } catch (...) {
        // Log warning
    }
}

void InSwapper::init() {
    auto input_dims = get_input_node_dims();
    if (input_dims.empty()) { throw std::runtime_error("Failed to get input node dims."); }

    // NCHW
    m_input_width = static_cast<int>(input_dims[0][2]);
    m_input_height = static_cast<int>(input_dims[0][3]);
    m_size = cv::Size(m_input_width, m_input_height);

    // Load ONNX model as a protobuf message for initializer
    onnx::ModelProto modelProto;
    std::ifstream input(get_loaded_model_path(), std::ios::binary);
    if (!modelProto.ParseFromIstream(&input)) {
        throw std::runtime_error("Failed to parse model protobuf.");
    }

    const onnx::TensorProto* initializer = nullptr;
    // Robustly find the 512x512 initializer (Arcface embedding transform)
    for (const auto& init : modelProto.graph().initializer()) {
        if (init.dims_size() == 2 && init.dims(0) == 512 && init.dims(1) == 512) {
            initializer = &init;
            break;
        }
    }

    if (!initializer) {
        // Fallback to last one if not found (legacy behavior)
        if (modelProto.graph().initializer_size() > 0) {
            initializer =
                &modelProto.graph().initializer(modelProto.graph().initializer_size() - 1);
        } else {
            throw std::runtime_error("No initializers found in model.");
        }
    }

    bool isFp16 = false;

    if (initializer->data_type() == onnx::TensorProto_DataType::TensorProto_DataType_FLOAT16) {
        isFp16 = true;
    }

    if (!isFp16) {
        if (initializer->float_data_size() > 0) {
            m_initializer_array.assign(initializer->float_data().begin(),
                                       initializer->float_data().end());
        } else if (!initializer->raw_data().empty()) {
            // Handle float data in raw_data
            std::string rawData = initializer->raw_data();
            auto data = reinterpret_cast<const float*>(rawData.data());
            m_initializer_array.assign(data, data + rawData.size() / sizeof(float));
        }
    } else {
        std::string rawData = initializer->raw_data();
        auto data = reinterpret_cast<const float*>(rawData.data());
        m_initializer_array.assign(data, data + rawData.size() / sizeof(float));
    }
    input.close();
}

cv::Mat InSwapper::swap_face(const SwapInput& input) {
    if (input.source_embedding.empty() || input.target_frame.empty()) { return {}; }
    if (input.target_faces_landmarks.empty()) { return input.target_frame.clone(); }

    if (!is_model_loaded()) { throw std::runtime_error("Model is not loaded!"); }
    if (m_initializer_array.empty()) { init(); }

    const auto& targetFrame = input.target_frame;
    std::vector<cv::Mat> croppedTargetFrames;
    std::vector<cv::Mat> affineMatrices;
    std::vector<cv::Mat> croppedResultFrames;

    // 1. Warp faces
    for (const auto& landmarks5 : input.target_faces_landmarks) {
        auto [croppedTargetFrame, affineMat] = warp_face_by_face_landmarks_5(
            targetFrame, landmarks5, get_warp_template(m_warp_template_type), m_size);
        croppedTargetFrames.emplace_back(croppedTargetFrame);
        affineMatrices.emplace_back(affineMat);
    }

    // 2. Apply Swap
    for (const auto& croppedTargetFrame : croppedTargetFrames) {
        croppedResultFrames.emplace_back(apply_swap(input.source_embedding, croppedTargetFrame));
    }

    // 3. Masking
    // TODO: Phase 3 - Restore MaskCompositor with Box/Occlusion/Region support
    cv::Mat resultFrame = targetFrame.clone();

    for (size_t i = 0; i < croppedResultFrames.size(); ++i) {
        // Prepare composition input
        MaskCompositor::CompositionInput maskInput;
        maskInput.size = m_size;
        maskInput.options = input.mask_options;
        maskInput.crop_frame = croppedTargetFrames[i];
        maskInput.occluder = m_occluder.get();
        maskInput.region_masker = m_region_masker.get();

        cv::Mat composedMask = MaskCompositor::compose(maskInput);

        // Paste back
        resultFrame =
            paste_back(resultFrame, croppedResultFrames[i], composedMask, affineMatrices[i]);
    }

    return resultFrame;
}

std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<float>, std::vector<int64_t>>
InSwapper::prepare_input(const domain::face::types::Embedding& source_embedding,
                         const cv::Mat& cropped_target_frame) const {
    // 1. Prepare Source Embedding
    std::vector<float> input_embedding_data;
    double norm = cv::norm(source_embedding, cv::NORM_L2);
    size_t lenFeature = source_embedding.size();
    input_embedding_data.resize(lenFeature);

    for (size_t i = 0; i < lenFeature; ++i) {
        double sum = 0.0f;
        for (size_t j = 0; j < lenFeature; ++j) {
            // Legacy behavior: M[j * len + i]
            sum += source_embedding.at(j) * m_initializer_array.at(j * lenFeature + i);
        }
        input_embedding_data.at(i) = static_cast<float>(sum / norm);
    }

    std::vector<int64_t> input_embedding_shape{1,
                                               static_cast<int64_t>(input_embedding_data.size())};

    // 2. Prepare Target Frame
    std::vector<cv::Mat> bgrChannels(3);
    cv::split(cropped_target_frame, bgrChannels);

    // Normalize: (x / 255.0 - mean) / std  => x * (1/(255*std)) - (mean/std)
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1.0 / (255.0 * m_standard_deviation[c]),
                                 -m_mean[c] / m_standard_deviation[c]);
    }

    int imageArea = cropped_target_frame.rows * cropped_target_frame.cols;
    std::vector<float> input_image_data(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);

    // CHW format: R, G, B
    memcpy(input_image_data.data(), (float*)bgrChannels[2].data, singleChnSize);             // R
    memcpy(input_image_data.data() + imageArea, (float*)bgrChannels[1].data, singleChnSize); // G
    memcpy(input_image_data.data() + imageArea * 2, (float*)bgrChannels[0].data,
           singleChnSize); // B

    std::vector<int64_t> input_image_shape = {1, 3, m_input_height, m_input_width};

    return std::make_tuple(std::move(input_embedding_data), std::move(input_embedding_shape),
                           std::move(input_image_data), std::move(input_image_shape));
}

cv::Mat InSwapper::process_output(const std::vector<Ort::Value>& output_tensors) const {
    if (output_tensors.empty()) return {};

    // Post-process
    const float* pdata = output_tensors[0].GetTensorData<float>();
    auto outsShape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

    // Handle dynamic shapes if necessary, but here it's likely fixed 1x3x128x128
    int outputHeight = static_cast<int>(outsShape[2]);
    int outputWidth = static_cast<int>(outsShape[3]);
    int channelStep = outputHeight * outputWidth;
    std::vector<cv::Mat> channelMats(3);
    // Legacy mapping: Plane 0 = R, Plane 2 = B.
    // Use clone() to ensure we own the data and can modify it
    channelMats[2] =
        cv::Mat(outputHeight, outputWidth, CV_32FC1, const_cast<float*>(pdata)).clone(); // R
    channelMats[1] =
        cv::Mat(outputHeight, outputWidth, CV_32FC1, const_cast<float*>(pdata) + channelStep)
            .clone(); // G
    channelMats[0] =
        cv::Mat(outputHeight, outputWidth, CV_32FC1, const_cast<float*>(pdata) + 2 * channelStep)
            .clone(); // B

    // Find Global Min/Max manually (proven to work)
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    int totalElements = outputHeight * outputWidth * 3;
    for (int i = 0; i < totalElements; ++i) {
        if (pdata[i] < minVal) minVal = pdata[i];
        if (pdata[i] > maxVal) maxVal = pdata[i];
    }

    float range = maxVal - minVal;
    if (range < 0.00001f) range = 1.0f;
    // float scale = 255.0f / range;
    // Wait, original code calculated `scale` but then used `mat *= 255.f;`. `scale` variable was
    // NOT used in original loop! Original code: float scale = 255.0f / range; for (auto& mat :
    // channelMats) {
    //    mat *= 255.f;
    // }
    // It seems `scale` calculation was dead code in original or I missed something.
    // Let's look at `inswapper.cpp` again. Step 411 line 221.
    // `float scale = 255.0f / range;`
    // Line 226: `mat *= 255.f;` (literal 255.f)
    // So `scale` was indeed unused. I should keep the behavior identical (multiply by 255).
    // But `range` calculation was also useless if `scale` not used.
    // However, I should assume the original code *intended* to use 255 directly as per observed
    // line 226.

    // BUT, is it possible `mat *= scale` was intended?
    // "Matrix transposition fixed the structure, so we trust the model's color balance now."
    // comment suggests maybe normalization changed. I will stick to `255.f` as per original source.

    for (auto& mat : channelMats) {
        // Simple scaling as the model output is in [0, 1] range.
        mat *= 255.f;

        // Clamp
        cv::threshold(mat, mat, 0, 0, cv::THRESH_TOZERO);
        cv::threshold(mat, mat, 255, 255, cv::THRESH_TRUNC);
    }

    cv::Mat resultMat;
    cv::merge(channelMats, resultMat);
    resultMat.convertTo(resultMat, CV_8UC3);
    return resultMat;
}

cv::Mat InSwapper::apply_swap(const Embedding& source_embedding,
                              const cv::Mat& cropped_target_frame) const {
    auto [input_embedding, embedding_shape, input_image, image_shape] =
        prepare_input(source_embedding, cropped_target_frame);

    std::vector<Ort::Value> inputTensors;

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    auto input_names = m_session->get_input_names();

    // Map inputs. We have two.
    for (const auto& inputName : input_names) {
        if (inputName == "source") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(
                memory_info, input_embedding.data(), input_embedding.size(), embedding_shape.data(),
                embedding_shape.size()));
        } else if (inputName == "target") {
            inputTensors.emplace_back(
                Ort::Value::CreateTensor<float>(memory_info, input_image.data(), input_image.size(),
                                                image_shape.data(), image_shape.size()));
        }
    }

    auto outputTensors = m_session->run(inputTensors);

    return process_output(outputTensors);
}

} // namespace domain::face::swapper
