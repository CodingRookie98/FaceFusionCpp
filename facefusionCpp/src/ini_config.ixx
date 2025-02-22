/**
 ******************************************************************************
 * @file           : config.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-17
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <vector>
#include <SimpleIni.h>

export module ini_config;
import core;
import logger;

export namespace ffc {
class ini_config {
public:
    explicit ini_config();
    ~ini_config() = default;

    bool loadConfig(const std::string &configPath = "./FaceFusionCpp.ini");

    [[nodiscard]] Core::Options getCoreOptions() const {
        return m_coreOptions;
    }
    [[nodiscard]] CoreTask getCoreRunOptions() const {
        return m_coreRunOptions;
    }

private:
    CSimpleIniA m_ini;
    std::shared_mutex m_sharedMutex;
    std::string m_configPath;
    std::shared_ptr<Logger> m_logger = Logger::getInstance();
    Core::Options m_coreOptions;
    CoreTask m_coreRunOptions;

    static std::array<int, 4> normalizePadding(const std::vector<int> &padding);
    static std::vector<int> parseStr2VecInt(const std::string &input);

    void general();
    void misc();
    void execution();
    void tensorrt();
    void memory();
    void faceAnalyser();
    void faceSelector();
    void faceMasker();
    void image();
    void video();
    void frameProcessors();
    static void tolower(std::string &str);
};
} // namespace ffc

