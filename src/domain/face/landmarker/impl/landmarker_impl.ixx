module;
#include <memory>
#include <string>
#include <opencv2/core.hpp>

export module domain.face.landmarker:impl;
import :api;

export namespace domain::face::landmarker {

class T2dfan : public IFaceLandmarker {
public:
    T2dfan();
    ~T2dfan() override;
    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override;
    LandmarkerResult detect(const cv::Mat& image, const cv::Rect2f& bbox) override;

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

class Peppawutz : public IFaceLandmarker {
public:
    Peppawutz();
    ~Peppawutz() override;
    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override;
    LandmarkerResult detect(const cv::Mat& image, const cv::Rect2f& bbox) override;

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

class T68By5 : public IFaceLandmarker {
public:
    T68By5();
    ~T68By5() override;
    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override;
    LandmarkerResult detect(const cv::Mat& image, const cv::Rect2f& bbox) override;
    domain::face::types::Landmarks expand_68_from_5(
        const domain::face::types::Landmarks& landmarks5) override;

private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace domain::face::landmarker
