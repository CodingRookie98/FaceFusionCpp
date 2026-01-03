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
import thread_pool;

namespace ffc::face_masker {

using namespace ai::model_manager;
using namespace infra;

FaceMaskerHub::FaceMaskerHub(const std::shared_ptr<Ort::Env>& env,
                             const ai::InferenceSession::Options& options) :
    m_env(env), m_options(options) {
}

FaceMaskerHub::~FaceMaskerHub() {
    std::unique_lock lock(m_sharedMutex);
    for (auto val : m_maskers | std::views::values) {
        delete val;
    }
    m_maskers.clear();
}

FaceMaskerBase* FaceMaskerHub::get_masker(const FaceMaskerHub::Type& type, const Model& model) {
    std::unique_lock lock(m_sharedMutex);
    if (m_maskers.contains(type)) {
        if (m_maskers[type] != nullptr) {
            return m_maskers[type];
        }
    }

    static std::shared_ptr<ModelManager> modelManager = ModelManager::get_instance();
    FaceMaskerBase* masker = nullptr;
    if (type == Type::Region) {
        masker = new FaceMaskerRegion(m_env);
        masker->load_model(modelManager->get_model_path(model), m_options);
    }
    if (type == Type::Occlusion) {
        masker = new Occlusion(m_env);
        masker->load_model(modelManager->get_model_path(model), m_options);
    }
    if (masker != nullptr) {
        m_maskers[type] = masker;
    }
    return m_maskers[type];
}

cv::Mat FaceMaskerHub::get_best_mask(const ArgsForGetBestMask& func_gbm_args) {
    std::vector<std::future<cv::Mat>> futures;
    std::vector<cv::Mat> masks;

    if (func_gbm_args.faceMaskersTypes.contains(Type::Box) && func_gbm_args.boxSize.has_value()
        && func_gbm_args.boxMaskBlur.has_value() && func_gbm_args.boxMaskPadding.has_value()) {
        futures.emplace_back(ThreadPool::Instance()->Enqueue(create_static_box_mask, func_gbm_args.boxSize.value(), func_gbm_args.boxMaskBlur.value(), func_gbm_args.boxMaskPadding.value()));
    }

    if (func_gbm_args.faceMaskersTypes.contains(Type::Occlusion) && func_gbm_args.occlusionFrame.has_value()
        && func_gbm_args.occluder_model.has_value()) {
        futures.emplace_back(ThreadPool::Instance()->Enqueue([&] {
            return create_occlusion_mask(*func_gbm_args.occlusionFrame.value(), func_gbm_args.occluder_model.value());
        }));
    }

    if (func_gbm_args.faceMaskersTypes.contains(Type::Region) && func_gbm_args.regionFrame.has_value()) {
        futures.emplace_back(ThreadPool::Instance()->Enqueue([&] {
            return create_region_mask(*func_gbm_args.regionFrame.value(), func_gbm_args.parser_model.value(),
                                      func_gbm_args.faceMaskerRegions.value());
        }));
    }

    for (auto& future : futures) {
        masks.emplace_back(future.get());
    }

    cv::Mat bestMask = get_best_mask(masks);
    return bestMask;
}

cv::Mat FaceMaskerHub::create_occlusion_mask(const cv::Mat& cropVisionFrame,
                                             const Model& occluder_model) {
    if (occluder_model != Model::xseg_1
        && occluder_model != Model::xseg_2) {
        throw std::runtime_error("Occlusion model not supported.");
    }
    const auto faceMaskerOcclusion = dynamic_cast<Occlusion*>(get_masker(Type::Occlusion, occluder_model));
    return faceMaskerOcclusion->createOcclusionMask(cropVisionFrame);
}

cv::Mat FaceMaskerHub::create_region_mask(const cv::Mat& inputImage,
                                          const Model& parser_model,
                                          const std::unordered_set<FaceMaskerRegion::Region>& regions) {
    if (parser_model != Model::bisenet_resnet_18
        && parser_model != Model::bisenet_resnet_34) {
        throw std::runtime_error("Region model not supported");
    }
    const auto faceMaskerRegion = dynamic_cast<FaceMaskerRegion*>(get_masker(Type::Region, parser_model));
    return faceMaskerRegion->createRegionMask(inputImage, regions);
}

cv::Mat FaceMaskerHub::create_static_box_mask(const cv::Size& cropSize, const float& faceMaskBlur, const std::array<int, 4>& faceMaskPadding) {
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

cv::Mat FaceMaskerHub::get_best_mask(const std::vector<cv::Mat>& masks) {
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
} // namespace ffc::face_masker