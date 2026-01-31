/**
 * @file checkpoint_manager.cpp
 * @brief Checkpoint manager implementation
 */
module;

#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <nlohmann/json.hpp>

module services.pipeline.checkpoint;

import foundation.infrastructure.logger;
import foundation.infrastructure.crypto; // sha1_string

namespace services::pipeline {

using json = nlohmann::json;
using foundation::infrastructure::crypto::sha1_string;
using foundation::infrastructure::logger::Logger;

// JSON serialization helpers
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CheckpointData, task_id, config_hash, last_completed_frame,
                                   total_frames, output_path, output_file_size, created_at,
                                   updated_at, version, checksum)

CheckpointManager::CheckpointManager(const std::filesystem::path& checkpoint_dir) :
    m_checkpoint_dir(checkpoint_dir),
    m_last_save_time(std::chrono::steady_clock::now() - std::chrono::hours{1}) {
    if (!std::filesystem::exists(m_checkpoint_dir)) {
        try {
            std::filesystem::create_directories(m_checkpoint_dir);
            Logger::get_instance()->debug(std::format(
                "[CheckpointManager] Created checkpoint directory: {}", m_checkpoint_dir.string()));
        } catch (const std::exception& e) {
            Logger::get_instance()->error(
                std::format("[CheckpointManager] Failed to create checkpoint directory {}: {}",
                            m_checkpoint_dir.string(), e.what()));
            throw;
        }
    }
}

bool CheckpointManager::save(const CheckpointData& data, std::chrono::seconds min_interval) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();
    if (now - m_last_save_time < min_interval) {
        return false; // Skip, too soon
    }

    force_save_internal(data);
    m_last_save_time = now;
    return true;
}

void CheckpointManager::force_save(const CheckpointData& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    force_save_internal(data);
}

void CheckpointManager::force_save_internal(const CheckpointData& data) {
    auto path = get_checkpoint_path(data.task_id);

    // Prepare data with updated timestamp and checksum
    CheckpointData save_data = data;
    save_data.updated_at = get_iso_timestamp();
    if (save_data.created_at.empty()) { save_data.created_at = save_data.updated_at; }
    save_data.checksum = ""; // Clear before calculating
    save_data.checksum = calculate_checksum(save_data);

    // Write to temp file first, then rename (atomic on most filesystems)
    auto temp_path = path;
    temp_path += ".tmp";

    try {
        std::ofstream ofs(temp_path);
        if (!ofs) {
            Logger::get_instance()->error(std::format(
                "[CheckpointManager] Failed to create checkpoint file: {}", temp_path.string()));
            return;
        }

        ofs << serialize(save_data);
        ofs.close();

        if (std::filesystem::exists(path)) { std::filesystem::remove(path); }
        std::filesystem::rename(temp_path, path);

        Logger::get_instance()->debug(
            std::format("[CheckpointManager] Saved checkpoint: task={}, frame={}/{}", data.task_id,
                        data.last_completed_frame, data.total_frames));
    } catch (const std::exception& e) {
        Logger::get_instance()->error(std::format(
            "[CheckpointManager] Error saving checkpoint for task {}: {}", data.task_id, e.what()));
    }
}

std::optional<CheckpointData> CheckpointManager::load(const std::string& task_id,
                                                      const std::string& config_hash) {
    auto path = get_checkpoint_path(task_id);

    if (!std::filesystem::exists(path)) { return std::nullopt; }

    try {
        std::ifstream ifs(path);
        if (!ifs) {
            Logger::get_instance()->warn(
                std::format("[CheckpointManager] Cannot read checkpoint file: {}", path.string()));
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << ifs.rdbuf();

        auto data = deserialize(buffer.str());
        if (!data) {
            Logger::get_instance()->warn(
                std::format("[CheckpointManager] Invalid checkpoint format: {}", path.string()));
            return std::nullopt;
        }

        // Verify integrity
        if (!verify_integrity(*data)) {
            Logger::get_instance()->error(std::format(
                "[CheckpointManager] Checkpoint integrity check failed: {}", path.string()));
            return std::nullopt;
        }

        // Verify config consistency (if hash provided)
        if (!config_hash.empty() && data->config_hash != config_hash) {
            Logger::get_instance()->warn(
                std::format("[CheckpointManager] Config mismatch - checkpoint config hash differs. "
                            "Starting fresh. task={}",
                            task_id));
            return std::nullopt;
        }

        Logger::get_instance()->info(
            std::format("[CheckpointManager] Loaded checkpoint: task={}, resume from frame {}/{}",
                        task_id, data->last_completed_frame + 1, data->total_frames));

        return data;
    } catch (const std::exception& e) {
        Logger::get_instance()->error(std::format(
            "[CheckpointManager] Error loading checkpoint for task {}: {}", task_id, e.what()));
        return std::nullopt;
    }
}

void CheckpointManager::cleanup(const std::string& task_id) {
    auto path = get_checkpoint_path(task_id);

    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
            Logger::get_instance()->info(
                std::format("[CheckpointManager] Cleaned up checkpoint: {}", task_id));
        }
    } catch (const std::exception& e) {
        Logger::get_instance()->error(std::format(
            "[CheckpointManager] Error cleaning up checkpoint for task {}: {}", task_id, e.what()));
    }
}

bool CheckpointManager::exists(const std::string& task_id) const {
    return std::filesystem::exists(get_checkpoint_path(task_id));
}

std::filesystem::path CheckpointManager::get_checkpoint_path(const std::string& task_id) const {
    return m_checkpoint_dir / (task_id + ".ckpt");
}

double CheckpointManager::calculate_progress(const CheckpointData& data) {
    if (data.total_frames <= 0) return 0.0;
    return static_cast<double>(data.last_completed_frame + 1) / data.total_frames * 100.0;
}

std::string CheckpointManager::serialize(const CheckpointData& data) const {
    json j = data;
    return j.dump(2); // Pretty print with 2-space indent
}

std::optional<CheckpointData> CheckpointManager::deserialize(const std::string& json_str) const {
    try {
        json j = json::parse(json_str);
        return j.get<CheckpointData>();
    } catch (const json::exception& e) {
        Logger::get_instance()->error(
            std::format("[CheckpointManager] JSON parse error: {}", e.what()));
        return std::nullopt;
    }
}

bool CheckpointManager::verify_integrity(const CheckpointData& data) const {
    CheckpointData verify_data = data;
    std::string saved_checksum = verify_data.checksum;
    verify_data.checksum = "";

    std::string calculated = calculate_checksum(verify_data);
    return saved_checksum == calculated;
}

std::string CheckpointManager::calculate_checksum(const CheckpointData& data) const {
    // Serialize without checksum field for hashing
    json j;
    j["task_id"] = data.task_id;
    j["config_hash"] = data.config_hash;
    j["last_completed_frame"] = data.last_completed_frame;
    j["total_frames"] = data.total_frames;
    j["output_path"] = data.output_path;
    j["output_file_size"] = data.output_file_size;
    j["created_at"] = data.created_at;
    j["updated_at"] = data.updated_at;
    j["version"] = data.version;

    return sha1_string(j.dump());
}

std::string CheckpointManager::get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

} // namespace services::pipeline
