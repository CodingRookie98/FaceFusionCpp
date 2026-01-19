module;
#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.swapper:inswapper;

import :impl_base;
import :types;
import domain.face.helper;
import domain.face.masker;

export namespace domain::face::swapper {

class InSwapper final : public FaceSwapperImplBase {
public:
    InSwapper();
    ~InSwapper() override = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options = {}) override;

    cv::Mat swap_face(const SwapInput& input) override;

private:
    void init();

    std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<float>, std::vector<int64_t>>
    prepare_input(const domain::face::types::Embedding& source_embedding,
                  const cv::Mat& cropped_target_frame) const;
    cv::Mat process_output(const std::vector<Ort::Value>& output_tensors) const;

    // Helper to orchestrate the swap for a single face
    cv::Mat apply_swap(const domain::face::types::Embedding& source_embedding,
                       const cv::Mat& cropped_target_frame) const;

private:
    cv::Size m_size{0, 0};
    int m_input_width = 0;
    int m_input_height = 0;
    std::vector<float> m_mean = {0.0f, 0.0f, 0.0f};
    std::vector<float> m_standard_deviation = {1.0f, 1.0f, 1.0f};
    std::vector<float> m_initializer_array;
    domain::face::helper::WarpTemplateType m_warp_template_type =
        domain::face::helper::WarpTemplateType::Arcface_128_v2;

    // Maskers
    std::unique_ptr<domain::face::masker::IFaceOccluder> m_occluder;
    std::unique_ptr<domain::face::masker::IFaceRegionMasker> m_region_masker;
};

} // namespace domain::face::swapper
