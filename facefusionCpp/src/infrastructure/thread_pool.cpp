/**
 ******************************************************************************
 * @file           : thread_pool.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 25-2-24
 ******************************************************************************
 */

module;
#include <memory>
#include <mutex>

module thread_pool;

namespace ffc::infra {

ThreadPool::ThreadPool(const unsigned int& thread_num) {
    if (thread_num <= 0 || thread_num > std::thread::hardware_concurrency()) {
        thread_pool_ = std::make_unique<dp::thread_pool<>>(std::thread::hardware_concurrency());
    } else {
        thread_pool_ = std::make_unique<dp::thread_pool<>>(thread_num);
    }
}

ThreadPool::~ThreadPool() {
    WaitForTasks();
}

std::shared_ptr<ThreadPool> ThreadPool::Instance() {
    static std::shared_ptr<ThreadPool> instance;
    static std::once_flag flag;
    std::call_once(flag, [&] { instance = std::make_shared<ThreadPool>(); });
    return instance;
}

void ThreadPool::WaitForTasks() {
    thread_pool_->wait_for_tasks();
}

auto ThreadPool::Size() const {
    return thread_pool_->size();
}

size_t ThreadPool::ClearTasks() {
    return thread_pool_->clear_tasks();
}

void ThreadPool::Reset(const unsigned int& thread_num) {
    WaitForTasks();
    if (thread_num <= 0 || thread_num > std::thread::hardware_concurrency()) {
        thread_pool_ = std::make_unique<dp::thread_pool<>>(std::thread::hardware_concurrency());
    } else {
        thread_pool_ = std::make_unique<dp::thread_pool<>>(thread_num);
    }
}

} // namespace ffc::infra