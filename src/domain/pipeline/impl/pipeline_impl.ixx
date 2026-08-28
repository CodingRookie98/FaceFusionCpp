/**
 * @file pipeline_impl.ixx
 * @brief Implementation of the multi-threaded processing pipeline
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <optional>
#include <map>
#include <mutex>
#include <semaphore>

export module domain.pipeline:impl;

import :api;
import :types;
import :queue;

export namespace domain::pipeline {

/**
 * @brief Concrete implementation of IPipeline
 * @details Uses a pool of worker threads to process frames from an input queue
 *          and provides processed frames in strict sequential order via an output queue.
 */
class Pipeline : public IPipeline {
public:
    /**
     * @brief Construct a new Pipeline with specific configuration
     */
    explicit Pipeline(PipelineConfig config) :
        m_config(config), m_input_queue(config.max_queue_size),
        m_output_queue(config.max_queue_size) {
        // GPU 并发闸门：限制同时处理帧的 worker 数（0 = 不限）
        m_gate_enabled = config.max_concurrent_gpu_tasks > 0;
        if (m_gate_enabled) { m_gpu_gate.release(config.max_concurrent_gpu_tasks); }
    }

    ~Pipeline() override { stop(); }

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    /**
     * @brief Add a processor to the pipeline (must be called before start())
     */
    void add_processor(std::shared_ptr<IFrameProcessor> processor) override {
        if (processor) { m_processors.push_back(processor); }
    }

    /**
     * @brief Spawn worker threads and start processing
     */
    void start() override {
        if (m_active.exchange(true)) return;

        for (int i = 0; i < m_config.worker_thread_count; ++i) {
            m_workers.emplace_back([this] { worker_loop(); });
        }
    }

    /**
     * @brief Signal workers to stop and join all threads
     */
    void stop() override {
        if (!m_active.exchange(false)) return;

        m_input_queue.shutdown();
        m_output_queue.shutdown();

        for (auto& worker : m_workers) {
            if (worker.joinable()) { worker.join(); }
        }
        m_workers.clear();
    }

    /**
     * @brief Push a frame into the input queue
     */
    void push_frame(FrameData frame) override {
        if (m_active) { m_input_queue.push(std::move(frame)); }
    }

    /**
     * @brief Pop a processed frame from the output queue
     */
    std::optional<FrameData> pop_frame() override { return m_output_queue.pop(); }

    /**
     * @brief Check if the pipeline is active
     */
    bool is_active() const override { return m_active; }

private:
    void worker_loop() {
        while (m_active) {
            // Optimization: Fetch batch of frames to reduce lock contention
            auto frames = m_input_queue.pop_batch(4); // Batch size 4 is a reasonable starting point

            if (frames.empty()) {
                // If batch is empty, check if queue is still active or shutdown
                if (!m_input_queue.is_active()) break;
                // Otherwise loop again (pop_batch waits, so no busy spin)
                continue;
            }

            for (auto& frame : frames) {
                // GPU 并发闸门：每帧处理前 acquire，作用域退出释放（含异常路径）
                const bool gated = !frame.is_end_of_stream && m_gate_enabled;
                if (gated) { m_gpu_gate.acquire(); }
                struct GateGuard {
                    std::counting_semaphore<>& sem;
                    const bool active;
                    explicit GateGuard(std::counting_semaphore<>& s, bool a) : sem(s), active(a) {}
                    ~GateGuard() {
                        if (active) sem.release();
                    }
                } guard{m_gpu_gate, gated};

                if (!frame.is_end_of_stream) {
                    for (auto& processor : m_processors) {
                        if (processor) { processor->process(frame); }
                    }
                }
                if (m_active) { push_to_output_ordered(std::move(frame)); }
            }
        }
    }

    void push_to_output_ordered(FrameData frame) {
        const std::scoped_lock kLock(m_reorder_mutex);

        if (frame.sequence_id == m_next_sequence_id) {
            m_output_queue.push(std::move(frame));
            m_next_sequence_id++;

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
            m_reorder_buffer.emplace(frame.sequence_id, std::move(frame));
        }
    }

    PipelineConfig m_config;
    ThreadSafeQueue<FrameData> m_input_queue;
    ThreadSafeQueue<FrameData> m_output_queue;

    std::vector<std::shared_ptr<IFrameProcessor>> m_processors;
    std::vector<std::jthread> m_workers;

    std::atomic<bool> m_active{false};

    std::mutex m_reorder_mutex;
    std::int64_t m_next_sequence_id = 0;
    std::map<std::int64_t, FrameData> m_reorder_buffer;

    // GPU 并发闸门（max_concurrent_gpu_tasks，0 = 不限）
    std::counting_semaphore<> m_gpu_gate{0};
    bool m_gate_enabled = false;
};

} // namespace domain::pipeline
