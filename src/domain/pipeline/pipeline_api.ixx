module;
#include <memory>
#include <optional>

/**
 * @file pipeline_api.ixx
 * @brief Pipeline interface definition
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.pipeline:api;

import :types;

export namespace domain::pipeline {

/**
 * @brief Interface for processing a single frame
 */
class IFrameProcessor {
public:
    virtual ~IFrameProcessor() = default;

    /**
     * @brief Process a frame
     * @details Can modify frame image (Swapper/Enhancer) or metadata (Detector)
     * @param frame Frame data to process
     */
    virtual void process(FrameData& frame) = 0;
};

/**
 * @brief Pipeline interface
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    /**
     * @brief Add a processor to the pipeline
     * @param processor Shared pointer to processor
     */
    virtual void add_processor(std::shared_ptr<IFrameProcessor> processor) = 0;

    /**
     * @brief Start the pipeline
     */
    virtual void start() = 0;

    /**
     * @brief Stop the pipeline
     */
    virtual void stop() = 0;

    /**
     * @brief Push a frame into the pipeline (Producer)
     * @param frame Frame data
     */
    virtual void push_frame(FrameData frame) = 0;

    /**
     * @brief Pop a processed frame from the pipeline (Consumer)
     * @return Optional frame data (empty if queue closed/empty)
     */
    virtual std::optional<FrameData> pop_frame() = 0;

    /**
     * @brief Check if pipeline is active
     * @return True if active
     */
    virtual bool is_active() const = 0;
};
} // namespace domain::pipeline
