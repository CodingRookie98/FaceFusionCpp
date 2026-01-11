/**
 ******************************************************************************
 * @file           : model_manager.h
 * @author         : CodingRookie
 * @brief          : Model management module for handling AI model information and operations
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

module;
#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

export module domain.ai.model_manager;

namespace domain::ai::model_manager {

using json = nlohmann::json;
/**
 * @brief Enumeration of supported AI models
 */
export enum class Model {
    /* -- face_enhancer begin -- */
    Gfpgan_12,
    Gfpgan_13,
    Gfpgan_14,
    Codeformer,
    /* -- face_enhancer end -- */

    /* -- face_swapper begin -- */
    Inswapper_128,
    Inswapper_128_fp16,
    /* -- face_swapper end -- */

    /* -- face_detector begin --  */
    Face_detector_retinaface,
    Face_detector_scrfd,
    Face_detector_yoloface,
    /* -- face_detector end --  */

    /* -- face_recognizer begin -- */
    Face_recognizer_arcface_w600k_r50,
    /* -- face_recognizer end -- */

    /* -- face_landmarker begin -- */
    Face_landmarker_68,
    Face_landmarker_68_5,
    Face_landmarker_peppawutz,
    /* -- face_landmarker end -- */

    /* -- faceClassfier begin -- */
    FairFace,
    /* -- faceClassfier end -- */

    /* -- face_masker begin -- */
    bisenet_resnet_18, // face_parser
    bisenet_resnet_34, // face_parser
    xseg_1,            // face_occluder
    xseg_2,            // face_occluder
    /* -- face_masker end -- */

    /* -- expression_restorer begin -- */
    Feature_extractor, // for live_portrait
    Motion_extractor,  // for live_portrait
    Generator,         // for live_portrait
    /* -- expression_restorer end -- */

    /* -- Frame_enhancer begin -- */
    Real_esrgan_x2,
    Real_esrgan_x2_fp16,
    Real_esrgan_x4,
    Real_esrgan_x4_fp16,
    Real_esrgan_x8,
    Real_esrgan_x8_fp16,
    Real_hatgan_x4,
    /* -- Frame_enhancer end -- */

    /* -- other begin -- */
    Unknown,
    /* -- other end -- */
};

/**
 * @brief Structure containing model information
 */
export struct ModelInfo {
    Model model;      ///< Model enumeration value
    std::string name; ///< Model name
    std::string path; ///< Model file path
    std::string url;  ///< Model download URL
};

/**
 * @brief Model manager class for handling AI model operations
 *
 * This class provides functionality to manage AI models including:
 * - Retrieving model information (path, URL, name)
 * - Downloading models
 * - Checking download status
 * - Model type classification
 */
export class ModelManager {
public:
    /**
     * @brief Construct a new ModelManager object
     * @param json_file_path Path to the models information JSON file
     */
    explicit ModelManager(const std::string& json_file_path = "./models_info.json");

    ~ModelManager() = default;

    /**
     * @brief Get the singleton instance of ModelManager
     * @param json_file_path Path to the models information JSON file
     * @return std::shared_ptr<ModelManager> Shared pointer to the singleton instance
     */
    static std::shared_ptr<ModelManager> get_instance(const std::string& json_file_path = "./models_info.json");

    /**
     * @brief Get complete model information
     * @param model Model enumeration value
     * @return ModelInfo Complete model information structure
     * @note If model is Model::Unknown, returns a ModelInfo structure with model field set to Model::Unknown and other fields as empty strings
     */
    [[nodiscard]] ModelInfo get_model_info(const Model& model) const;

    /**
     * @brief Get model file path
     * @param model Model enumeration value
     * @return std::string Model file path
     * @note Returns empty string if model file doesn't exist or model is Model::Unknown
     */
    [[nodiscard]] std::string get_model_path(const Model& model) const;

    /**
     * @brief Get model download URL
     * @param model Model enumeration value
     * @return std::string Model download URL
     * @note Returns empty string if model is Model::Unknown
     */
    [[nodiscard]] std::string get_model_url(const Model& model) const;
    /**
     * @brief Get model name from model enumeration
     * @param model Model enumeration value
     * @return std::string Model name as string
     * @note Returns empty string if model is Model::Unknown
     */
    std::string get_model_name(const Model& model);
    /**
     * @brief Download a specific model
     * @param model Model enumeration value
     * @return bool True if download succeeded, false otherwise
     * @note If model is Model::Unknown, function does nothing and returns true
     */
    [[nodiscard]] bool download_model(const Model& model) const;

    /**
     * @brief Check if a model is already downloaded
     * @param model Model enumeration value
     * @return bool True if model is downloaded, false otherwise
     * @note Returns false if model is Model::Unknown
     */
    [[nodiscard]] bool is_downloaded(const Model& model) const;

    /**
     * @brief Check if a model is a face swapper model
     * @param model Model enumeration value
     * @return bool True if model is a face swapper, false otherwise
     */
    [[nodiscard]] static bool is_face_swapper_model(const Model& model) {
        return model == Model::Inswapper_128 || model == Model::Inswapper_128_fp16;
    }

    /**
     * @brief Check if a model is a face enhancer model
     * @param model Model enumeration value
     * @return bool True if model is a face enhancer, false otherwise
     */
    [[nodiscard]] static bool is_face_enhancer_model(const Model& model) {
        return model == Model::Gfpgan_12 || model == Model::Gfpgan_13 || model == Model::Gfpgan_14 || model == Model::Codeformer;
    }
    /**
     * @brief Get path to models information JSON file
     * @return std::string Path to models information JSON file
     */
    [[nodiscard]] std::string get_model_json_file_path() const { return m_json_file_path; }

private:
    std::string m_json_file_path;                           ///< Path to models information JSON file
    std::unordered_map<Model, ModelInfo> m_models_info_map; ///< Map of model enumeration values to ModelInfo structures
};

/**
 * @brief Serialize Model enum to JSON
 * @param j JSON object to write to
 * @param model Model enum value to serialize
 */
export void to_json(json& j, const Model& model);

/**
 * @brief Deserialize Model enum from JSON
 * @param j JSON object to read from
 * @param model Model enum value to deserialize
 */
export void from_json(const json& j, Model& model);

/**
 * @brief Serialize ModelInfo to JSON
 * @param j JSON object to write to
 * @param model_info ModelInfo object to serialize
 */
export void to_json(json& j, const ModelInfo& model_info);

/**
 * @brief Deserialize ModelInfo from JSON
 * @param j JSON object to read from
 * @param model_info ModelInfo object to deserialize
 */
export void from_json(const json& j, ModelInfo& model_info);

} // namespace domain::ai::model_manager
