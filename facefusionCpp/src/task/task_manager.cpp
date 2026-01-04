/**
 ******************************************************************************
 * @file           : task_manager.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 2025/12/26
 ******************************************************************************
 */

module;
#include <mutex>
#include <utility>
#include <boost/process.hpp>

module task_manager;
import utils;
import file_system;
import vision;
import processor_hub;
import model_manager;

namespace ffc::task::task_manager {
using namespace core;
using namespace ai;
using namespace infra;
using namespace media;

using namespace ffc::ai::model_manager;

TaskManager& TaskManager::get_instance() {
    static TaskManager instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        instance.m_logger->trace("TaskManager instance created.");
        auto thread = std::jthread([&]() {
            instance.run_tasks();
        });
        thread.detach();
    });
    return instance;
}

void TaskManager::set_core(std::shared_ptr<Core> core) {
    m_core = std::move(core);
    m_logger->trace("TaskManager.core set.");
}

std::string TaskManager::get_current_task_id() {
    std::lock_guard lock_current_task_id(m_mutex_current_task_id);
    return m_current_task_id;
}

std::string TaskManager::submit_task(Task task) {
    std::string task_id;
    if (prepare_task(task)) {
        task_id = utils::generate_uuid();
    }
    if (task_id.empty()) {
        return task_id;
    }

    {
        std::unique_lock w_lock_task_map(m_mutex_task_map);
        m_task_map[task_id] = task;
    }

    {
        std::unique_lock w_lock_task_queue(m_mutex_task_queue);
        m_task_queue.push(task_id);
    }

    return task_id;
}

bool TaskManager::prepare_task(Task& task) const {
    if (task.target_paths.empty()) {
        m_logger->error("TaskManager.prepare_task: target_paths is empty.");
        return false;
    }
    if (task.output.path.empty()) {
        task.output.path = file_system::get_current_path();
    } else {
        if (!file_system::is_dir(task.output.path)) {
            m_logger->warn("TaskManager.prepare_task: output.path is not a directory. Replace with default output directory.");
            task.output.path = file_system::get_current_path();
        }
    }

    std::vector<std::string> target_paths;
    for (const auto& path : task.target_paths) {
        if (file_system::is_file(path)) {
            if (vision::is_image(path) || vision::is_video(path)) {
                target_paths.push_back(path);
            }
        } else {
            auto files = file_system::list_files(path);
            target_paths.insert(target_paths.end(), files.begin(), files.end());
        }
    }
    task.target_paths = std::move(target_paths);

    for (auto& [type, model, parameters] : task.processors_info) {
        if (type == ProcessorMajorType::FaceSwapper) {
            if (!ModelManager::is_face_swapper_model(model)) {
                m_logger->warn(
                    std::format("TaskManager.prepare_task: Face swapper model ({}) not found. Replace with default model ({}).",
                                ModelManager::get_instance()->get_model_name(model),
                                ModelManager::get_instance()->get_model_name(model_manager::Model::Inswapper_128_fp16)));
                model = model_manager::Model::Inswapper_128_fp16;
            } else {
            }
        } else if (type == ProcessorMajorType::FaceEnhancer) {
            if (!ModelManager::is_face_enhancer_model(model)) {
                m_logger->warn(
                    std::format("TaskManager.prepare_task: Face enhancer model ({}) not found. Replace with default model ({}).",
                                ModelManager::get_instance()->get_model_name(model),
                                ModelManager::get_instance()->get_model_name(model_manager::Model::Codeformer)));
                model = model_manager::Model::Codeformer;
            } else {
                if (!parameters.contains("blend_factor")) {
                    m_logger->warn(std::format("TaskManager.prepare_task: Face enhancer model ({}) blend_factor not found. Replace with default value (0.8).",
                                               ModelManager::get_instance()->get_model_name(model)));
                    parameters["blend_factor"] = "0.8";
                }
            }
        } else if (type == ProcessorMajorType::ExpressionRestorer) {
            // Todo
            // if (!ModelManager::is_expression_restorer_model(model)) {
            //     logger_->warn(
            //         std::format("TaskManager.prepare_task: Expression restorer model ({}) not found. Replace with default model ({}).",
            //                     ModelManager::get_model_name(model),
            //                     ModelManager::get_model_name(model_manager::Model::Gfpgan_14)));
            //     model = model_manager::Model::Gfpgan_14;
            // }
        }
    }

    return true;
}

void TaskManager::run_tasks() {
    while (true) {
        {
            std::unique_lock lock_current_task_id(m_mutex_current_task_id);
            m_current_task_id.clear();
        }

        std::string task_id;
        {
            bool is_task_queue_empty = false;
            {
                std::lock_guard lock_task_queue(m_mutex_task_queue);
                if (m_task_queue.empty()) {
                    is_task_queue_empty = true;
                }
            }

            if (is_task_queue_empty) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            } else {
                std::lock_guard lock_task_queue(m_mutex_task_queue);
                task_id = m_task_queue.front();
                m_task_queue.pop();
            }
        }
        if (task_id.empty()) {
            continue;
        }

        Task task;
        {
            std::lock_guard lock_task_map(m_mutex_task_map);
            task = m_task_map[task_id];
        }
        {
            std::lock_guard lock_current_task_id(m_mutex_current_task_id);
            m_current_task_id = task_id;
        }
        // Todo 接收任务结果，更新任务状态
        m_core->run_task(task);
        {
            std::lock_guard lock_task_map(m_mutex_task_map);
            m_task_map.erase(task_id);
        }
    }
}
} // namespace ffc::task::task_manager