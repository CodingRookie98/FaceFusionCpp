/**
 ******************************************************************************
 * @file           : face_masker_hub.mxx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <ranges>
#include <future>
#include <opencv2/opencv.hpp>

module face_masker_hub;
import model_manager;

namespace ffc::faceMasker {
FaceMaskerHub::FaceMaskerHub(const std::shared_ptr<Ort::Env>& env,
                             const InferenceSession::Options& options) :
    m_env(env), m_options(options) {
}

FaceMaskerHub::~FaceMaskerHub() {
    std::unique_lock lock(m_sharedMutex);
    for (auto val : m_maskers | std::views::values) {
        delete val;
    }
    m_maskers.clear();
}

FaceMaskerBase* FaceMaskerHub::getMasker(const FaceMaskerHub::Type& type, const ModelManager::Model& model) {
    std::unique_lock lock(m_sharedMutex);
    if (m_maskers.contains(type)) {
        if (m_maskers[type] != nullptr) {
            return m_maskers[type];
        }
    }

    static std::shared_ptr<ModelManager> modelManager = ModelManager::getInstance();
    FaceMaskerBase* masker = nullptr;
    if (type == Type::Region) {
        masker = new FaceMaskerRegion(m_env);
        masker->loadModel(modelManager->getModelPath(model), m_options);
    }
    if (type == Type::Occlusion) {
        masker = new Occlusion(m_env);
        masker->loadModel(modelManager->getModelPath(model), m_options);
    }
    if (masker != nullptr) {
        m_maskers[type] = masker;
    }
    return m_maskers[type];
}

cv::Mat FaceMaskerHub::getBestMask(const ArgsForGetBestMask& func_gbm_args) {
    std::vector<std::future<cv::Mat>> futures;
    std::vector<cv::Mat> masks;

    if (func_gbm_args.faceMaskersTypes.contains(Type::Box) && func_gbm_args.boxSize.has_value()
        && func_gbm_args.boxMaskBlur.has_value() && func_gbm_args.boxMaskPadding.has_value()) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskerHub::createStaticBoxMask, func_gbm_args.boxSize.value(), func_gbm_args.boxMaskBlur.value(), func_gbm_args.boxMaskPadding.value()));
    }

    if (func_gbm_args.faceMaskersTypes.contains(Type::Occlusion) && func_gbm_args.occlusionFrame.has_value()
        && func_gbm_args.occluder_model.has_value()) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskerHub::createOcclusionMask, this,
                                        *func_gbm_args.occlusionFrame.value(),
                                        func_gbm_args.occluder_model.value()));
    }

    if (func_gbm_args.faceMaskersTypes.contains(Type::Region) && func_gbm_args.regionFrame.has_value()) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskerHub::createRegionMask, this,
                                        *func_gbm_args.regionFrame.value(),
                                        func_gbm_args.parser_model.value(),
                                        func_gbm_args.faceMaskerRegions.value()));
    }

    for (auto& future : futures) {
        masks.emplace_back(future.get());
    }

    cv::Mat bestMask = getBestMask(masks);
    return bestMask;
}

cv::Mat FaceMaskerHub::createOcclusionMask(const cv::Mat& cropVisionFrame,
                                           const ModelManager::Model& occluder_model) {
    if (occluder_model != ModelManager::Model::xseg_1
        && occluder_model != ModelManager::Model::xseg_2) {
        throw std::runtime_error("Occlusion model not supported.");
    }
    const auto faceMaskerOcclusion = dynamic_cast<Occlusion*>(getMasker(Type::Occlusion, occluder_model));
    return faceMaskerOcclusion->createOcclusionMask(cropVisionFrame);
}

cv::Mat FaceMaskerHub::createRegionMask(const cv::Mat& inputImage,
                                        const ModelManager::Model& parser_model,
                                        const std::unordered_set<FaceMaskerRegion::Region>& regions) {
    if (parser_model != ModelManager::Model::bisenet_resnet_18
        && parser_model != ModelManager::Model::bisenet_resnet_34) {
        throw std::runtime_error("Region model not supported");
    }
    const auto faceMaskerRegion = dynamic_cast<FaceMaskerRegion*>(getMasker(Type::Region, parser_model));
    return faceMaskerRegion->createRegionMask(inputImage, regions);
}

cv::Mat FaceMaskerHub::createStaticBoxMask(const cv::Size& cropSize, const float& faceMaskBlur, const std::array<int, 4>& faceMaskPadding) {
    const int blurAmount = static_cast<int>(cropSize.width * 0.5 * faceMaskBlur);
    const int blurArea = std::max(blurAmount / 2, 1);

    cv::Mat boxMask(cropSize, CV_32F, cv::Scalar(1.0f));

    const int paddingTop = std::max(blurArea, static_cast<int>(cropSize.height * faceMaskPadding.at(0) / 100.0));
    const int paddingRight = std::max(blurArea, static_cast<int>(cropSize.width * faceMaskPadding.at(1) / 100.0));
    const int paddingBottom = std::max(blurArea, static_cast<int>(cropSize.height * faceMaskPadding.at(2) / 100.0));
    const int paddingLeft = std::max(blurArea, static_cast<int>(cropSize.width * faceMaskPadding.at(3) / 100.0));

    boxMask(cv::Range(0, paddingTop), cv::Range::all()) = 0;
    boxMask(cv::Range(cropSize.height - paddingBottom, cropSize.height), cv::Range::all()) = 0;
    boxMask(cv::Range::all(), cv::Range(0, paddingLeft)) = 0;
    boxMask(cv::Range::all(), cv::Range(cropSize.width - paddingRight, cropSize.width)) = 0;

    if (blurAmount > 0) {
        cv::GaussianBlur(boxMask, boxMask, cv::Size(0, 0), blurAmount * 0.25);
    }

    return boxMask;
}

cv::Mat FaceMaskerHub::getBestMask(const std::vector<cv::Mat>& masks) {
    if (masks.empty()) {
        throw std::invalid_argument("The input vector is empty.");
    }

    // Initialize the minMask with the first mask in the vector
    cv::Mat minMask = masks[0].clone();

    for (size_t i = 1; i < masks.size(); ++i) {
        // Check if the current mask has the same size and type as minMask
        if (masks[i].size() != minMask.size() || masks[i].type() != minMask.type()) {
            throw std::invalid_argument("[FaceMasker] All masks must have the same size and type.");
        }
        cv::min(minMask, masks[i], minMask);
    }
    minMask.setTo(0, minMask < 0);
    minMask.setTo(1, minMask > 1);

    return minMask;
}
} // namespace ffc::faceMasker