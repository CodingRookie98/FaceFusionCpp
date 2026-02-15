module;
#include <vector>
#include <array>
#include <future>
#include <memory>
#include <unordered_set>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

export module domain.face.masker:compositor;

import domain.face; // for types
import :api;        // for IFaceOccluder, IFaceRegionMasker
import foundation.infrastructure.thread_pool;

export namespace domain::face::masker {

class MaskCompositor {
public:
    struct CompositionInput {
        cv::Size size;
        domain::face::types::MaskOptions options; // Updated to use centralized types
        cv::Mat crop_frame;

        // Raw pointers used as non-owning references.
        IFaceOccluder* occluder = nullptr;
        IFaceRegionMasker* region_masker = nullptr;
    };

    /**
     * @brief Compute and compose multiple masks in parallel
     */
    static cv::Mat compose(const CompositionInput& input) {
        std::vector<std::future<cv::Mat>> futures;
        const auto& opts = input.options;
        auto& pool = foundation::infrastructure::thread_pool::ThreadPool::instance();

        // 1. Box Mask Task
        bool box_mask_enabled = false;
        for (auto t : opts.mask_types) {
            if (t == domain::face::types::MaskType::Box) { box_mask_enabled = true; }
        }

        if (box_mask_enabled) {
            futures.emplace_back(pool.enqueue([&]() {
                return create_box_mask(input.size, opts.box_mask_blur, opts.box_mask_padding);
            }));
        }

        // 2. Occlusion Mask Task
        bool occlusion_mask_enabled = false;
        for (auto t : opts.mask_types) {
            if (t == domain::face::types::MaskType::Occlusion) { occlusion_mask_enabled = true; }
        }

        if (occlusion_mask_enabled && input.occluder != nullptr && !input.crop_frame.empty()) {
            futures.emplace_back(pool.enqueue([&]() -> cv::Mat {
                const cv::Mat kOcc = input.occluder->create_occlusion_mask(input.crop_frame);
                // Invert: 255 (occluded) -> 0 (keep original), 0 (clear) -> 255 (swap)
                // Use OpenCV subtraction to ensure result is cv::Mat not MatExpr
                cv::Mat inverted;
                cv::subtract(cv::Scalar(255), kOcc, inverted);
                return inverted;
            }));
        }

        // 3. Region Mask Task
        bool region_mask_enabled = false;
        for (auto t : opts.mask_types) {
            if (t == domain::face::types::MaskType::Region) { region_mask_enabled = true; }
        }

        if (region_mask_enabled && input.region_masker != nullptr && !input.crop_frame.empty()) {
            futures.emplace_back(pool.enqueue([&]() {
                using FaceRegion = domain::face::types::FaceRegion;
                const std::unordered_set<FaceRegion> kActiveRegions(opts.regions.begin(),
                                                                    opts.regions.end());
                return input.region_masker->create_region_mask(input.crop_frame, kActiveRegions);
            }));
        }

        // Collect results
        std::vector<cv::Mat> masks;
        masks.reserve(futures.size());
        for (auto& f : futures) { masks.emplace_back(f.get()); }

        if (masks.empty()) {
            // If no mask generated, return full white (swap everything)
            // Legacy face_helper::paste_back expects CV_32FC1 (0.0-1.0).
            return cv::Mat::ones(input.size, CV_32FC1);
        }

        // Fusion (Min)
        cv::Mat final_mask = masks[0].clone();

        // Ensure first mask is float 0-1
        if (final_mask.type() == CV_8UC1) {
            final_mask.convertTo(final_mask, CV_32FC1, 1.0 / 255.0);
        }

        for (size_t i = 1; i < masks.size(); ++i) {
            cv::Mat next_mask = masks[i];
            if (next_mask.type() == CV_8UC1) {
                next_mask.convertTo(next_mask, CV_32FC1, 1.0 / 255.0);
            }

            // Resize if needed
            if (next_mask.size() != final_mask.size()) {
                cv::resize(next_mask, next_mask, final_mask.size());
            }

            cv::min(final_mask, next_mask, final_mask);
        }

        // Final Blur for Edge Blending (Task 3.3)
        // Smooth transitions between combined masks
        int kernel_size = std::max(
            3, static_cast<int>(static_cast<float>(input.size.width) * 0.025F)); // 2.5% of width
        if (kernel_size % 2 == 0) { kernel_size++; }
        cv::GaussianBlur(final_mask, final_mask, cv::Size(kernel_size, kernel_size), 0);

        return final_mask;
    }

private:
    static cv::Mat create_box_mask(const cv::Size& size, float blur,
                                   const std::array<int, 4>& padding) {
        int blur_amount = static_cast<int>(static_cast<float>(size.width) * 0.5F * blur);
        const int kBlurArea = std::max(blur_amount / 2, 1);

        cv::Mat mask = cv::Mat::ones(size, CV_32FC1);

        const int kPadTop =
            std::max(kBlurArea, static_cast<int>(static_cast<float>(size.height)
                                                 * static_cast<float>(padding[0]) / 100.0F));
        const int kPadRight =
            std::max(kBlurArea, static_cast<int>(static_cast<float>(size.width)
                                                 * static_cast<float>(padding[1]) / 100.0F));
        const int kPadBot =
            std::max(kBlurArea, static_cast<int>(static_cast<float>(size.height)
                                                 * static_cast<float>(padding[2]) / 100.0F));
        const int kPadLeft =
            std::max(kBlurArea, static_cast<int>(static_cast<float>(size.width)
                                                 * static_cast<float>(padding[3]) / 100.0F));

        // Set borders to 0
        if (kPadTop > 0) { mask(cv::Rect(0, 0, size.width, kPadTop)) = 0; }
        if (kPadBot > 0) { mask(cv::Rect(0, size.height - kPadBot, size.width, kPadBot)) = 0; }
        if (kPadLeft > 0) { mask(cv::Rect(0, 0, kPadLeft, size.height)) = 0; }
        if (kPadRight > 0) {
            mask(cv::Rect(size.width - kPadRight, 0, kPadRight, size.height)) = 0;
        }

        // Blur
        if (blur_amount > 0) {
            if (blur_amount % 2 == 0) { blur_amount++; }
            cv::GaussianBlur(mask, mask, cv::Size(0, 0), blur_amount * 0.25);
        }

        return mask;
    }
};

} // namespace domain::face::masker
