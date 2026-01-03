/**
 ******************************************************************************
 * @file           : face_masker_region.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_masker_hub:region;
import :face_masker_base;

export namespace ffc::face_masker {
class FaceMaskerRegion final : public FaceMaskerBase {
public:
    explicit FaceMaskerRegion(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceMaskerRegion() override = default;

    enum class Region {
        Skin = 1,
        LeftEyebrow = 2,
        RightEyebrow = 3,
        LeftEye = 4,
        RightEye = 5,
        Glasses = 6,
        Nose = 10,
        Mouth = 11,
        UpperLip = 12,
        LowerLip = 13
    };

    [[nodiscard]] cv::Mat createRegionMask(const cv::Mat &inputImage, const std::unordered_set<Region> &regions = getAllRegions()) const;

    void load_model(const std::string& modelPath, const Options& options) override;

    static std::unordered_set<Region> getAllRegions() {
        return {Region::Skin, Region::LeftEyebrow, Region::RightEyebrow, Region::LeftEye, Region::RightEye, Region::Glasses, Region::Nose, Region::Mouth, Region::UpperLip, Region::LowerLip};
    };

private:
    int m_inputHeight{0};
    int m_inputWidth{0};

    [[nodiscard]] std::vector<float> getInputImageData(const cv::Mat &image) const;
};

} // namespace ffc::faceMasker