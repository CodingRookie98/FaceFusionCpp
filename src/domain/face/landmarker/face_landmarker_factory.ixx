module;
#include <memory>

export module domain.face.landmarker:factory;
import :api;

export namespace domain::face::landmarker {

/**
 * @brief Landmarker 类型
 */
enum class LandmarkerType {
    _2DFAN,    ///< 2DFAN4 模型 (68点)
    Peppawutz, ///< Peppawutz 模型 (68点)
    _68By5     ///< 从5点关键点预测68点
};

/**
 * @brief 创建 Landmarker 实例
 * @param type Landmarker 类型
 * @return Landmarker 实例指针
 */
std::unique_ptr<IFaceLandmarker> create_landmarker(LandmarkerType type);

} // namespace domain::face::landmarker
