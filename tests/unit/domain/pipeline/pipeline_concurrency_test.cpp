#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <opencv2/core.hpp>

import domain.pipeline;

using namespace domain::pipeline;
using namespace std::chrono_literals;

// T2: GPU 并发信号量闸门（max_concurrent_gpu_tasks）

namespace {

class CountingProcessor : public IFrameProcessor {
public:
    void process(FrameData& frame) override {
        const int cur = ++active;
        update_max(cur);
        // 模拟 GPU 推理耗时
        std::this_thread::sleep_for(15ms);
        --active;
    }

    std::atomic<int> active{0};
    std::atomic<int> peak{0};

private:
    void update_max(int value) {
        int current = peak.load();
        while (value > current && !peak.compare_exchange_weak(current, value)) {}
    }
};

FrameData MakeFrame(int seq) {
    FrameData f;
    f.sequence_id = seq;
    f.image = cv::Mat::zeros(64, 64, CV_8UC3);
    return f;
}

} // namespace

TEST(PipelineConcurrencyTest, ConcurrencyLimitedByMaxGpuTasks) {
    PipelineConfig cfg;
    cfg.worker_thread_count = 4;
    cfg.max_concurrent_gpu_tasks = 2;
    cfg.max_queue_size = 32;
    Pipeline pipeline(cfg);

    auto proc = std::make_shared<CountingProcessor>();
    pipeline.add_processor(proc);
    pipeline.start();

    for (int i = 0; i < 8; ++i) { pipeline.push_frame(MakeFrame(i)); }
    FrameData eos;
    eos.is_end_of_stream = true;
    eos.sequence_id = 8; // 必须连续，否则 EOS 卡在 reorder buffer
    pipeline.push_frame(std::move(eos));

    int popped = 0;
    while (auto result = pipeline.pop_frame()) {
        if (result->is_end_of_stream) break;
        ++popped;
    }
    pipeline.stop();

    EXPECT_EQ(popped, 8);
    EXPECT_LE(proc->peak.load(), 2) << "并发 GPU 任务数应被闸门限制在 2";
}

TEST(PipelineConcurrencyTest, ConcurrencyUnlimitedWhenZero) {
    PipelineConfig cfg;
    cfg.worker_thread_count = 4;
    cfg.max_concurrent_gpu_tasks = 0; // 0 = 不限
    cfg.max_queue_size = 32;
    Pipeline pipeline(cfg);

    auto proc = std::make_shared<CountingProcessor>();
    pipeline.add_processor(proc);
    pipeline.start();

    for (int i = 0; i < 8; ++i) { pipeline.push_frame(MakeFrame(i)); }
    FrameData eos;
    eos.is_end_of_stream = true;
    eos.sequence_id = 8;
    pipeline.push_frame(std::move(eos));

    while (auto result = pipeline.pop_frame()) {
        if (result->is_end_of_stream) break;
    }
    pipeline.stop();

    // 4 worker 无闸门时并发应 > 闸门限制（2），证明闸门关闭生效；
    // 不断言 == 4（峰值依赖 worker 调度的同时活跃性）
    EXPECT_GT(proc->peak.load(), 2);
}

TEST(PipelineConcurrencyTest, OrderPreservedWithGating) {
    PipelineConfig cfg;
    cfg.worker_thread_count = 4;
    cfg.max_concurrent_gpu_tasks = 2;
    cfg.max_queue_size = 32;
    Pipeline pipeline(cfg);

    auto proc = std::make_shared<CountingProcessor>();
    pipeline.add_processor(proc);
    pipeline.start();

    for (int i = 0; i < 16; ++i) { pipeline.push_frame(MakeFrame(i)); }
    FrameData eos;
    eos.is_end_of_stream = true;
    eos.sequence_id = 16;
    pipeline.push_frame(std::move(eos));

    std::vector<int> seqs;
    while (auto result = pipeline.pop_frame()) {
        if (result->is_end_of_stream) break;
        seqs.push_back(static_cast<int>(result->sequence_id));
    }
    pipeline.stop();

    ASSERT_EQ(seqs.size(), 16u);
    for (size_t i = 0; i < seqs.size(); ++i) {
        EXPECT_EQ(seqs[i], static_cast<int>(i)) << "输出应严格保序";
    }
}