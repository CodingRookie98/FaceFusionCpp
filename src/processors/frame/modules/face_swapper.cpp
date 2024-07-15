/**
 ******************************************************************************
 * @file           : face_swapper.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

#include "face_swapper.h"

namespace Ffc {
FaceSwapper::FaceSwapper(const std::shared_ptr<Ort::Env> &env,
                         const std::shared_ptr<FaceAnalyser> &faceAnalyser,
                         const std::shared_ptr<FaceMasker> &faceMasker,
                         const std::shared_ptr<nlohmann::json> &modelsInfoJson) :
    OrtSession(env), m_modelsInfoJson(modelsInfoJson) {
    m_faceAnalyser = faceAnalyser;
    m_faceMasker = faceMasker;
}

void FaceSwapper::processImage(const std::vector<std::string> &sourcePaths,
                               const std::string &targetPath,
                               const std::string &outputPath) {
    Typing::Faces referenceFaces;
    std::vector<cv::Mat> sourceFrames = Ffc::Vision::readStaticImages(sourcePaths);
    std::shared_ptr<Typing::Face> sourceFace = m_faceAnalyser->getAverageFace(sourceFrames);
    auto targetFrame = Ffc::Vision::readStaticImage(targetPath);

    auto resultFrame = processFrame(referenceFaces, *sourceFace, targetFrame);
    Ffc::Vision::writeImage(*resultFrame, outputPath);
}

std::shared_ptr<Typing::VisionFrame> FaceSwapper::processFrame(const Typing::Faces &referenceFaces,
                                                               const Face &sourceFace,
                                                               const VisionFrame &targetFrame) {
    if (m_faceAnalyser == nullptr) {
        throw std::runtime_error("Face analyser is not set");
    }

    std::shared_ptr<Typing::VisionFrame> resultFrame = std::make_shared<Typing::VisionFrame>(targetFrame);
    if (Globals::faceSelectorMode == Typing::EnumFaceSelectorMode::FS_Many) {
        auto manyTargetFaces = m_faceAnalyser->getManyFaces(targetFrame);
        if (manyTargetFaces != nullptr && !manyTargetFaces->empty()) {
            for (auto &targetFace : *manyTargetFaces) {
                resultFrame = swapFace(sourceFace, targetFace, *resultFrame);
            }
        }
    } else if (Globals::faceSelectorMode == Typing::EnumFaceSelectorMode::FS_One) {
        auto targetFace = m_faceAnalyser->getOneFace(targetFrame);
        if (targetFace != nullptr) {
            resultFrame = swapFace(sourceFace, *targetFace, *resultFrame);
        }
    } else if (Globals::faceSelectorMode == Typing::EnumFaceSelectorMode::FS_Reference) {
        // Todo Reference
    }

    return resultFrame;
}

std::shared_ptr<Typing::VisionFrame> FaceSwapper::swapFace(const Face &sourceFace,
                                                           const Face &targetFace,
                                                           const VisionFrame &targetFrame) {
    if (m_faceSwapperModel == nullptr || *m_faceSwapperModel != Globals::faceSwapperModel) {
        m_faceSwapperModel = std::make_shared<Typing::EnumFaceSwapperModel>(Globals::faceSwapperModel);
        switch (Globals::faceSwapperModel) {
        case Typing::InSwapper_128:
            m_modelName = "inswapper_128";
            break;
        case Typing::InSwapper_128_fp16:
            m_modelName = "inswapper_128_fp16";
            break;
        default:
            break;
        }
        std::string modelPath = m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("path");

        // 检查modelPath是否存在, 不存在则下载
        if (!FileSystem::fileExists(modelPath)) {
            bool downloadSuccess = Downloader::downloadFileFromURL(m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("url"),
                                                                   "./models");
            if (!downloadSuccess) {
                throw std::runtime_error("Failed to download the model file: " + modelPath);
            }
        }
        
        if (m_modelName == "inswapper_128" || m_modelName == "inswapper_128_fp16") {
            // Load ONNX model as a protobuf message
            onnx::ModelProto modelProto;
            std::ifstream input(modelPath, std::ios::binary);
            if (!modelProto.ParseFromIstream(&input)) {
                throw std::runtime_error("Failed to load model.");
            }
            // Access the initializer
            const onnx::TensorProto &initializer = modelProto.graph().initializer(modelProto.graph().initializer_size() - 1);
            // Convert initializer to an array
            m_initializerArray.clear();
            m_initializerArray.assign(initializer.float_data().begin(), initializer.float_data().end());
        } else {
            m_initializerArray.clear();
        }

        this->createSession(modelPath);
        init();
    }

    auto resultFrame = this->applySwap(sourceFace, targetFace, targetFrame);
    return resultFrame;
}

void FaceSwapper::init() {
    m_inputHeight = (int)m_inputNodeDims[0][2];
    m_inputWidth = (int)m_inputNodeDims[0][3];

    auto templateName = m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("template");
    auto fVec = m_modelsInfoJson->at("faceHelper").at("warpTemplate").at(templateName).get<std::vector<float>>();
    for (int i = 0; i < fVec.size(); i += 2) {
        m_warpTemplate.emplace_back(fVec.at(i), fVec.at(i + 1));
    }

    m_mean = m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("mean").get<std::vector<float>>();

    m_standardDeviation = m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("standard_deviation").get<std::vector<float>>();
    auto sizeVec = m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("size").get<std::vector<int>>();
    m_size = cv::Size(sizeVec.at(0), sizeVec.at(1));
}

std::shared_ptr<Typing::VisionFrame> FaceSwapper::applySwap(const Face &sourceFace, const Face &targetFace, const VisionFrame &targetFrame) {
    auto croppedTargetFrameAndAffineMat = FaceHelper::warpFaceByFaceLandmarks5(
        targetFrame, targetFace.faceLandMark5_68, m_warpTemplate, m_size);
    auto preparedTargetFrameBGR = Ffc::FaceSwapper::prepareCropVisionFrame(std::get<0>(*croppedTargetFrameAndAffineMat), m_mean, m_standardDeviation);

    auto cropMasks = Ffc::FaceSwapper::getCropMasks(std::get<0>(*croppedTargetFrameAndAffineMat),
                                                    std::get<0>(*croppedTargetFrameAndAffineMat).size(),
                                                    Globals::faceMaskBlur, Globals::faceMaskPadding);

    // Create input tensors
    std::vector<Ort::Value> inputTensors;
    std::string modelType = m_modelsInfoJson->at("faceSwapperModels").at(m_modelName).at("type").get<std::string>();
    for (const auto &inputName : m_inputNames) {
        if (std::string(inputName) == "source") {
            if (modelType == "blendswap" || modelType == "uniface") {
                // Todo 完善blendSWap和uniface
            } else {
                static auto inputEmbeddingData = this->prepareSourceEmbedding(sourceFace);
                std::vector<int64_t> inputEmbeddingShape = {1, (int64_t)inputEmbeddingData->size()};
                inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, inputEmbeddingData->data(), inputEmbeddingData->size(), inputEmbeddingShape.data(), inputEmbeddingShape.size()));
            }
        } else if (std::string(inputName) == "target") {
            static auto inputImageData = this->prepareCropFrameData(*preparedTargetFrameBGR);
            std::vector<int64_t> inputImageShape = {1, 3, m_inputHeight, m_inputWidth};
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, inputImageData->data(), inputImageData->size(), inputImageShape.data(), inputImageShape.size()));
        }
    }

    Ort::RunOptions runOptions;
    auto outputTensor = m_session->Run(runOptions, m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_outputNames.data(), m_outputNames.size());

    float *pdata = outputTensor[0].GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    const int outputHeight = outsShape[2];
    const int outputWidth = outsShape[3];

    const int channelStep = outputHeight * outputWidth;
    std::vector<cv::Mat> channelMats(3);
    // Create matrices for each channel and scale/clamp values
    channelMats[2] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata);                   // R
    channelMats[1] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + channelStep);     // G
    channelMats[0] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + 2 * channelStep); // B
    for (auto &mat : channelMats) {
        mat *= 255.f;
        mat.setTo(0, mat < 0);
        mat.setTo(255, mat > 255);
    }
    // Merge the channels into a single matrix
    cv::Mat resultMat;
    cv::merge(channelMats, resultMat);

    for (auto &cropMask : *cropMasks) {
        cropMask.setTo(0, cropMask < 0);
        cropMask.setTo(1, cropMask > 1);
    }

    auto cropMask = FaceMasker::getBestMask(*cropMasks);

    auto dstImage = FaceHelper::pasteBack(targetFrame, resultMat, *cropMask,
                                          std::get<1>(*croppedTargetFrameAndAffineMat));

    return dstImage;
}

void FaceSwapper::setFaceAnalyser(const std::shared_ptr<FaceAnalyser> &faceAnalyser) {
    m_faceAnalyser = faceAnalyser;
}

std::shared_ptr<Typing::VisionFrame> FaceSwapper::prepareCropVisionFrame(const VisionFrame &visionFrame, const std::vector<float> &mean, const std::vector<float> &standDeviation) {
    cv::Mat bgrImage = visionFrame.clone();
    std::vector<cv::Mat> bgrChannels(3);
    split(bgrImage, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels.at(c), CV_32FC1, 1 / (255.0 * standDeviation.at(c)),
                                 -mean.at(c) / (float)standDeviation.at(c));
    }

    cv::Mat processedBGR;
    cv::merge(bgrChannels, processedBGR);

    return std::make_shared<Typing::VisionFrame>(processedBGR);
}

std::shared_ptr<std::vector<cv::Mat>> FaceSwapper::getCropMasks(const VisionFrame &visionFrame, const cv::Size &cropSize, const float &faceMaskBlur, const Padding &faceMaskPadding) {
    auto cropMaskList = std::make_shared<std::vector<cv::Mat>>();
    if (Globals::faceMaskerTypeSet.contains(Typing::EnumFaceMaskerType::FM_Box)) {
        auto boxMask = FaceMasker::createStaticBoxMask(cropSize,
                                                       Globals::faceMaskBlur, Globals::faceMaskPadding);
        cropMaskList->push_back(*boxMask);
    } else if (Globals::faceMaskerTypeSet.contains(Typing::EnumFaceMaskerType::FM_Occlusion)) {
        auto occlusionMask = m_faceMasker->createOcclusionMask(visionFrame);
        cropMaskList->push_back(*occlusionMask);
    } else if (Globals::faceMaskerTypeSet.contains(Typing::EnumFaceMaskerType::FM_Region)) {
        auto regionMask = m_faceMasker->createRegionMask(visionFrame);
        cropMaskList->push_back(*regionMask);
    }
    return cropMaskList;
}

std::shared_ptr<std::vector<float>> FaceSwapper::prepareSourceEmbedding(const Face &sourceFace) {
    double norm = cv::norm(sourceFace.embedding, cv::NORM_L2);
    int lenFeature = sourceFace.embedding.size();
    std::vector<float> result(lenFeature);
    for (int i = 0; i < lenFeature; ++i) {
        double sum = 0.0f;
        for (int j = 0; j < lenFeature; ++j) {
            sum += sourceFace.embedding.at(j)
                   * m_initializerArray.at(j * lenFeature + i);
        }
        result.at(i) = (float)(sum / norm);
    }
    return std::make_shared<std::vector<float>>(std::move(result));
}

std::shared_ptr<std::vector<float>> FaceSwapper::prepareCropFrameData(const VisionFrame &cropFrame) const {
    cv::Mat bgrImage = cropFrame.clone();
    std::vector<cv::Mat> bgrChannels(3);
    split(bgrImage, bgrChannels);
    const int imageArea = this->m_inputHeight * this->m_inputWidth;
    std::vector<float> inputImageData(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize);                 // R
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);     // G
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize); // B
    return std::make_shared<std::vector<float>>(std::move(inputImageData));
}

} // namespace Ffc