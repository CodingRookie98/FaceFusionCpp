/**
 * @file pipeline_api.ixx
 * @brief Interfaces for the processing pipeline and processors
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <memory>
#include <optional>

export module domain.pipeline:api;

import :types;

export namespace domain::pipeline {

/**
 * @brief Interface for processing a single frame
 * @details Concrete processors (e.g. Swapper, Enhancer) implement this to perform
 *          work on image data or metadata.
 */
class IFrameProcessor {
public:
    virtual ~IFrameProcessor() = default;

    /**
     * @brief Process a single frame
     * @param frame Frame data structure containing image and metadata
     */
    virtual void process(FrameData& frame) = 0;

    /**
     * @brief Ensure the processor's resources (e.g., models) are loaded
     */
    virtual void ensure_loaded() {}
};

/**
 * @brief Main interface for the multi-stage processing pipeline
 * @details Manages a sequence of IFrameProcessor instances and executes them
 *          across multiple worker threads.
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    /**
     * @brief Add a processor to the end of the pipeline
     */
    virtual void add_processor(std::shared_ptr<IFrameProcessor> processor) = 0;

    /**
     * @brief Start the pipeline execution engine
     */
    virtual void start() = 0;

    /**
     * @brief Stop the pipeline and wait for workers to finish
     */
    virtual void stop() = 0;

    /**
     * @brief Push a new frame for processing (Producer API)
     */
    virtual void push_frame(FrameData frame) = 0;

    /**
     * @brief Retrieve a fully processed frame (Consumer API)
     * @return Optional frame data, or nullopt if stream ended
     */
    virtual std::optional<FrameData> pop_frame() = 0;

    /**
     * @brief Check if the pipeline is currently running
     */
    virtual bool is_active() const = 0;
};

} // namespace domain::pipeline
