/**
 ******************************************************************************
 * @file           : task_manager.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 2025/12/26
 ******************************************************************************
 */

module;
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

export module task_manager;
import task;
import logger;
import core;

namespace ffc::task::task_manager {

using namespace ffc::core;
using namespace ffc::infra;

class TaskManager {
public:
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    static TaskManager& get_instance();
    void set_core(std::shared_ptr<Core> core);
    // 提交任务到任务队列, 返回任务ID。成功返回任务ID，失败返回空字符串。
    std::string submit_task(Task task);
    // 获取当前正在运行的任务ID。如果没有任务在运行，则返回空字符串。
    std::string get_current_task_id();

private:
    TaskManager() = default;
    ~TaskManager() = default;

    // 准备任务, 并且检查任务是否合法, 合法则返回true, 否则返回false。
    bool prepare_task(Task& task) const;
    // 运行任务队列中的任务。每次运行一个任务，就会从任务队列中取出一个任务，并运行。
    // 这个函数会在单独的线程中运行, 直到程序退出时才会结束。
    [[noreturn]] void run_tasks();
    std::shared_ptr<Logger> m_logger = Logger::get_instance();
    std::shared_ptr<Core> m_core;
    // 键为任务ID, 值为任务对象。
    std::unordered_map<std::string, Task> m_task_map;
    std::mutex m_mutex_task_map;
    // 任务队列, 用于存储待运行的任务。
    std::queue<std::string> m_task_queue;
    /**
     * @brief 任务队列互斥锁
     *
     * 用于保护任务队列的线程安全访问。
     */
    std::mutex m_mutex_task_queue;
    // 当前正在运行的任务ID。如果没有任务在运行，则为空字符串。
    std::string m_current_task_id;
    std::mutex m_mutex_current_task_id;
};
} // namespace ffc::task::task_manager