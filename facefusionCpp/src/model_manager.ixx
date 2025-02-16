/**
 ******************************************************************************
 * @file           : model_manager.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */
module;
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

export module model_manager;

export namespace ffc {

class ModelManager {
public:
    explicit ModelManager(const std::string& modelsInfoJsonPath = "./modelsInfo.json");
    ~ModelManager() = default;

    static std::shared_ptr<ModelManager> getInstance(const std::string& modelsInfoJsonPath = "./modelsInfo.json");

    enum class Model {
        // faceEnhancer
        Gfpgan_12,
        Gfpgan_13,
        Gfpgan_14,
        Codeformer,

        // faceSwapper
        Inswapper_128,
        Inswapper_128_fp16,
        // faceDetector
        Face_detector_retinaface,
        Face_detector_scrfd,
        Face_detector_yoloface,
        // faceRecognizer
        Face_recognizer_arcface_w600k_r50,
        // faceLandmarker
        Face_landmarker_68,
        Face_landmarker_68_5,
        Face_landmarker_peppawutz,
        // faceClassfier
        FairFace,

        // faceMasker
        bisenet_resnet_18, // face_parser
        bisenet_resnet_34, // face_parser
        xseg_1,            // face_occluder
        xseg_2,            // face_occluder

        // expressionRestorer
        Feature_extractor,
        Motion_extractor,
        Generator,
        // FrameEnhancer
        Real_esrgan_x2,
        Real_esrgan_x2_fp16,
        Real_esrgan_x4,
        Real_esrgan_x4_fp16,
        Real_esrgan_x8,
        Real_esrgan_x8_fp16,
        Real_hatgan_x4,
    };

    enum ModelInfoType {
        Path,
        Url,
    };

    void* getModelInfo(const Model& model, const ModelInfoType& modelInfoType,
                       const bool& skipDownload = false, std::string* errMsg = nullptr) const;
    std::string getModelPath(const Model& model, const bool& skipDownload = false, std::string* errMsg = nullptr) const;
    std::string getModelUrl(const Model& model, std::string* errMsg = nullptr) const;
    [[nodiscard]] std::unordered_set<std::string> getModelsUrl() const;
    [[nodiscard]] bool downloadAllModel() const;

private:
    std::string m_modelsInfoJsonPath;
    // nlohmann::json_abi_v3_11_3::json m_modelsInfoJson;
    std::unordered_map<ModelInfoType, std::string> m_modelInfoTypeMap = {
        {Path, "path"},
        {Url, "url"},
    };

    static std::string getModelName(const Model& model);
    //    bool checkModel(const std::string &modelPath, const std::string &modelUrl, std::string *errMsg = nullptr) const;
};

} // namespace ffc