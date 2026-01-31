module;
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.enhancer:code_former;
import :impl_base;
import :types;
import :api;
import domain.face.helper;

export namespace domain::face::enhancer {

class CodeFormer final : public FaceEnhancerImplBase {
public:
    CodeFormer();
    ~CodeFormer() override = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options = {}) override;

    cv::Mat enhance_face(const cv::Mat& target_crop) override;

    cv::Size get_model_input_size() const override { return m_size; }

private:
    int m_input_height = 0;
    int m_input_width = 0;
    cv::Size m_size{0, 0};

    std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<double>, std::vector<int64_t>>
    prepare_input(const cv::Mat& cropped_frame) const;
    cv::Mat process_output(const std::vector<Ort::Value>& output_tensors) const;
    cv::Mat apply_enhance(const cv::Mat& cropped_frame) const;
};

} // namespace domain::face::enhancer
