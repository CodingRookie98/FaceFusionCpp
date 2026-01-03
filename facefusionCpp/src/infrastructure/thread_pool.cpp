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
        m_thread_pool = std::make_unique<dp::thread_pool<>>(std::thread::hardware_concurrency());
    } else {
        m_thread_pool = std::make_unique<dp::thread_pool<>>(thread_num);
    }
}

ThreadPool::~ThreadPool() {
    wait_for_tasks();
}

std::shared_ptr<ThreadPool> ThreadPool::instance() {
    static std::shared_ptr<ThreadPool> instance;
    static std::once_flag flag;
    std::call_once(flag, [&] { instance = std::make_shared<ThreadPool>(); });
    return instance;
}

void ThreadPool::wait_for_tasks() {
    m_thread_pool->wait_for_tasks();
}

auto ThreadPool::size() const {
    return m_thread_pool->size();
}

size_t ThreadPool::clear_tasks() {
    return m_thread_pool->clear_tasks();
}

void ThreadPool::reset(const unsigned int& thread_num) {
    wait_for_tasks();
    if (thread_num <= 0 || thread_num > std::thread::hardware_concurrency()) {
        m_thread_pool = std::make_unique<dp::thread_pool<>>(std::thread::hardware_concurrency());
    } else {
        m_thread_pool = std::make_unique<dp::thread_pool<>>(thread_num);
    }
}

} // namespace ffc::infra