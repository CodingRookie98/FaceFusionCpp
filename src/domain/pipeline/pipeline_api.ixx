module;
#include <memory>
#include <optional>

export module domain.pipeline:api;

import :types;

export namespace domain::pipeline {

class IFrameProcessor {
public:
    virtual ~IFrameProcessor() = default;

    // 处理一帧数据。
    // 可以修改 frame.image (例如 Swapper/Enhancer)
    // 可以修改 frame.metadata (例如 Detector)
    virtual void process(FrameData& frame) = 0;
};

class IPipeline {
public:
    virtual ~IPipeline() = default;

    virtual void add_processor(std::shared_ptr<IFrameProcessor> processor) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    // 生产者接口
    virtual void push_frame(FrameData frame) = 0;

    // 消费者接口
    virtual std::optional<FrameData> pop_frame() = 0;

    virtual bool is_active() const = 0;
};
} // namespace domain::pipeline
