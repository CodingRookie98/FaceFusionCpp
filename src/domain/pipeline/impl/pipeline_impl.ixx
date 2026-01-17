module;
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <optional>
#include <map>
#include <mutex>

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

            if (!frame.is_end_of_stream) {
                for (auto& processor : m_processors) {
                    if (processor) { processor->process(frame); }
                }
            }

            if (m_active) { push_to_output_ordered(std::move(frame)); }
        }
    }

    // Ensures frames are pushed to output queue in strictly increasing sequence_id order
    void push_to_output_ordered(FrameData frame) {
        std::lock_guard<std::mutex> lock(m_reorder_mutex);

        // If frame is what we expect next, push it and check pending buffer
        if (frame.sequence_id == m_next_sequence_id) {
            m_output_queue.push(std::move(frame));
            m_next_sequence_id++;

            // Drain pending buffer
            while (!m_reorder_buffer.empty()) {
                auto it = m_reorder_buffer.begin();
                if (it->first == m_next_sequence_id) {
                    m_output_queue.push(std::move(it->second));
                    m_reorder_buffer.erase(it);
                    m_next_sequence_id++;
                } else {
                    break;
                }
            }
        } else {
            // Not next, buffer it
            // Note: If multiple workers process, frame.sequence_id > m_next_sequence_id is normal.
            // If frame.sequence_id < m_next_sequence_id, it's a duplicate or error, but we
            // buffer/ignore it. Assuming unique sequence_ids.
            m_reorder_buffer.emplace(frame.sequence_id, std::move(frame));
        }
    }

    PipelineConfig m_config;
    ThreadSafeQueue<FrameData> m_input_queue;
    ThreadSafeQueue<FrameData> m_output_queue;

    std::vector<std::shared_ptr<IFrameProcessor>> m_processors;
    std::vector<std::jthread> m_workers;

    std::atomic<bool> m_active{false};

    // Reordering logic
    std::mutex m_reorder_mutex;
    long long m_next_sequence_id = 0;
    std::map<long long, FrameData> m_reorder_buffer;
};
} // namespace domain::pipeline
