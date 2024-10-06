/**
 ******************************************************************************
 * @file           : core.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_CORE_H_
#define FACEFUSIONCPP_SRC_CORE_H_

#include <onnxruntime_cxx_api.h>
#include "face_analyser/face_analyser.h"
#include "processor_base.h"
#include "face_maskers.h"
#include "config.h"
#include "logger.h"
#include "face_store.h"

namespace Ffc {
class Core {
public:
    Core();
    ~Core();

    void run();
    void conditionalProcess();
    void processImages(const std::vector<std::string> &targetImagePaths, const std::vector<std::string> &outputImagePaths);
    bool processVideo(const std::string &videoPath, const std::string &outputVideoPath, const bool &skipAudio = false);
    bool processVideoBySegments(const std::string &videoPath, const std::string &outputVideoPath, const unsigned int &duration);
    void processVideos(const std::vector<std::string> &videoPaths, const std::vector<std::string> &outputVideoPaths);
    void processVideosBySegments(const std::vector<std::string> &videoPaths,
                                 const std::vector<std::string> &outputVideoPaths,
                                 const unsigned int &secondDuration);

private:
    std::shared_ptr<Config> m_config;
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<Ort::Env> m_env;
    std::shared_ptr<FaceAnalyser> m_faceAnalyser;
    std::shared_ptr<FaceMaskers> m_faceMaskers;
    std::unordered_map<ProcessorBase::ProcessorType, ProcessorBase *> m_processorPtrMap;
    std::shared_ptr<FaceStore> m_faceStore = FaceStore::getInstance();
    std::string_view m_sourceFacesStr = "sourceFaces";
    std::string_view m_referenceFacesStr = "referenceFaces";
    std::string_view m_sourceAverageFace = "sourceAverageFace";

    [[nodiscard]] ProcessorBase *createProcessor(const ProcessorBase::ProcessorType &processorType) const;
    void processReferenceFaces();
    void processSourceFaces();
    void conditionalProcessImages(const std::unordered_set<std::string> &targetImagePaths);
    void conditionalProcessVideos(const std::unordered_set<std::string> &videoPaths);
    bool processImage(const std::string &targetImagePath, const std::string &originalTargetImagePath,
                      const std::string &outputImagePath, ProcessorBase *processor);
};
} // namespace Ffc

#endif // FACEFUSIONCPP_SRC_CORE_H_
