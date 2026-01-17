module;
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <optional>

export module domain.pipeline:impl;

import :api;
import :types;
import :queue;

export namespace domain::pipeline {

class Pipeline : public IPipeline {
public:
    explicit Pipeline(PipelineConfig config) :
        m_config(config), m_input_queue(config.max_queue_size),
        m_output_queue(config.max_queue_size) {}

    ~Pipeline() override { stop(); }

    void add_processor(std::shared_ptr<IFrameProcessor> processor) override {
        if (processor) { m_processors.push_back(processor); }
    }

    void start() override {
        if (m_active.exchange(true)) return; // Already running

        for (int i = 0; i < m_config.worker_thread_count; ++i) {
            m_workers.emplace_back([this] { worker_loop(); });
        }
    }

    void stop() override {
        if (!m_active.exchange(false)) return; // Already stopped

        m_input_queue.shutdown();
        m_output_queue.shutdown(); // Optional: allow draining output? usually safer to shutdown

        for (auto& worker : m_workers) {
            if (worker.joinable()) { worker.join(); }
        }
        m_workers.clear();
    }

    void push_frame(FrameData frame) override {
        if (m_active) { m_input_queue.push(std::move(frame)); }
    }

    std::optional<FrameData> pop_frame() override { return m_output_queue.pop(); }

    bool is_active() const override { return m_active; }

private:
    void worker_loop() {
        while (m_active) {
            auto frame_opt = m_input_queue.pop();
            if (!frame_opt) {
                // Queue shutdown or empty & shutdown
                break;
            }

            FrameData frame = std::move(*frame_opt);

            // End of stream marker? Pass it through but don't process?
            // Or maybe process it if it carries data.
            // Assuming EOS might still carry the last frame or just a signal.
            // Let's assume EOS marker handles independently or flows through.

            if (!frame.is_end_of_stream) {
                for (auto& processor : m_processors) {
                    if (processor) { processor->process(frame); }
                }
            }

            if (m_active) { m_output_queue.push(std::move(frame)); }
        }
    }

    PipelineConfig m_config;
    ThreadSafeQueue<FrameData> m_input_queue;
    ThreadSafeQueue<FrameData> m_output_queue;

    std::vector<std::shared_ptr<IFrameProcessor>> m_processors;
    std::vector<std::jthread> m_workers;

    std::atomic<bool> m_active{false};
};
} // namespace domain::pipeline
