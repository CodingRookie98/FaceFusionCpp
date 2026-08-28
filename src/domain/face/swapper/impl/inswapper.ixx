module;
#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include <mutex>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.swapper:inswapper;

import :impl_base;
import :types;
import domain.face.helper;
export namespace domain::face::swapper {

class InSwapper final : public FaceSwapperImplBase {
public:
    InSwapper();
    ~InSwapper() override = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override;

    cv::Mat swap_face(cv::Mat target_crop, const std::vector<float>& source_embedding) override;

    [[nodiscard]] cv::Size get_model_input_size() const override { return m_size; }

    /**
     * @brief Number of embedding transform computations (diagnostic/cache hit check)
     */
    [[nodiscard]] size_t embedding_transform_count() const { return m_transform_count; }

private:
    void init();

    [[nodiscard]] std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<float>,
                             std::vector<int64_t>>
    prepare_input(const std::vector<float>& embedding_transform,
                  const cv::Mat& cropped_target_frame) const;
    [[nodiscard]] cv::Mat process_output(const std::vector<Ort::Value>& output_tensors) const;

    /**
     * @brief Compute the source embedding transform (embedding x initializer matrix / norm)
     * @details O(512^2) per call; results are cached in swap_face across frames
     */
    [[nodiscard]] std::vector<float> compute_embedding_transform(
        const domain::face::types::Embedding& source_embedding) const;

    // Helper to orchestrate the swap for a single face
    [[nodiscard]] cv::Mat apply_swap(const std::vector<float>& embedding_transform,
                                     const cv::Mat& cropped_target_frame) const;

private:
    cv::Size m_size{0, 0};
    int m_input_width = 0;
    int m_input_height = 0;
    std::vector<float> m_mean = {0.0F, 0.0F, 0.0F};
    std::vector<float> m_standard_deviation = {1.0F, 1.0F, 1.0F};
    std::vector<float> m_initializer_array;
    std::once_flag m_init_flag;

    // Embedding transform cache (task-invariant; avoids O(512^2) per face per frame)
    mutable std::mutex m_transform_mutex;
    std::vector<float> m_cached_embedding;
    std::vector<float> m_cached_transform;
    size_t m_transform_count = 0;
};

} // namespace domain::face::swapper
