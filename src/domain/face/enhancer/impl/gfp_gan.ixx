module;
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.enhancer:gfp_gan;
import :impl_base;
// import domain.face.masker;
import domain.face.helper;

export namespace domain::face::enhancer {

class GfpGan final : public FaceEnhancerImplBase {
public:
    GfpGan();
    ~GfpGan() override = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options = {}) override;

    cv::Mat enhance_face(const EnhanceInput& input) override;

private:
    int m_input_height = 0;
    int m_input_width = 0;
    cv::Size m_size{0, 0};
    domain::face::helper::WarpTemplateType m_warp_template_type =
        domain::face::helper::WarpTemplateType::Ffhq_512;
    // std::unique_ptr<domain::face::masker::IFaceMasker> m_face_masker;

    std::tuple<std::vector<float>, std::vector<int64_t>> prepare_input(
        const cv::Mat& cropped_frame) const;
    cv::Mat process_output(const std::vector<Ort::Value>& output_tensors) const;
    cv::Mat apply_enhance(const cv::Mat& cropped_frame) const;
};

} // namespace domain::face::enhancer
