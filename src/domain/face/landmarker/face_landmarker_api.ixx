module;
#include <opencv2/core.hpp>
#include <string>
#include <vector>

export module domain.face.landmarker:api;
import domain.face;
import foundation.ai.inference_session;

export namespace domain::face::landmarker {

/**
 * @brief 人脸关键点检测结果
 */
struct LandmarkerResult {
    domain::face::types::Landmarks landmarks; ///< 检测到的关键点
    float score;                              ///< 置信度得分
};

/**
 * @brief 人脸关键点检测器接口
 */
class IFaceLandmarker {
public:
    virtual ~IFaceLandmarker() = default;

    /**
     * @brief 加载模型
     * @param model_path 模型路径
     * @param options 推理会话选项
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    /**
     * @brief 检测关键点
     * @param image 输入图像
     * @param bbox 人脸边界框
     * @return LandmarkerResult 检测结果
     */
    virtual LandmarkerResult detect(const cv::Mat& image, const cv::Rect2f& bbox) = 0;

    /**
     * @brief 从5点关键点扩展到68点 (专门用于 68By5 模型)
     * @param landmarks5 5点关键点
     * @return 68点关键点
     */
    virtual domain::face::types::Landmarks expand_68_from_5(
        const domain::face::types::Landmarks& landmarks5) {
        return {};
    }
};

} // namespace domain::face::landmarker
