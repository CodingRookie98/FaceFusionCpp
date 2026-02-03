/**
 * @file checkpoint_manager.ixx
 * @brief Checkpoint management for resumable video processing
 * @author CodingRookie
 * @date 2026-01-31
 * @see design.md Section 5.9 - Checkpointing
 */
module;

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <filesystem>
#include <cstdint>
#include <mutex>

export module services.pipeline.checkpoint;

import foundation.infrastructure.logger;

export namespace services::pipeline {

/**
 * @brief Checkpoint data structure for task resumption
 * @details Stored as JSON file: checkpoints/{task_id}.ckpt
 */
struct CheckpointData {
    // ─────────────────────────────────────────────────────────────────────────
    // Core State
    // ─────────────────────────────────────────────────────────────────────────
    std::string task_id;               ///< Unique task identifier
    std::string config_hash;           ///< SHA1 hash of task config (for consistency check)
    int64_t last_completed_frame = -1; ///< Last successfully processed frame index
    int64_t total_frames = 0;          ///< Total frames in video

    // ─────────────────────────────────────────────────────────────────────────
    // Output Tracking
    // ─────────────────────────────────────────────────────────────────────────
    std::string output_path;      ///< Current output file path
    int64_t output_file_size = 0; ///< Size of partial output file (optional validation)

    // ─────────────────────────────────────────────────────────────────────────
    // Metadata
    // ─────────────────────────────────────────────────────────────────────────
    std::string created_at;      ///< ISO 8601 timestamp of creation
    std::string updated_at;      ///< ISO 8601 timestamp of last update
    std::string version = "1.0"; ///< Checkpoint format version
    std::string checksum;        ///< SHA1 of serialized data (for integrity)
};

/**
 * @brief Manages checkpoint persistence for resumable processing
 * @details Handles:
 *          - Periodic checkpoint saving (configurable interval)
 *          - Checkpoint loading and validation
 *          - Automatic cleanup on successful completion
 */
class CheckpointManager {
public:
    /**
     * @brief Construct checkpoint manager
     * @param checkpoint_dir Directory to store checkpoint files
     * @throws std::runtime_error if directory cannot be created
     */
    explicit CheckpointManager(const std::filesystem::path& checkpoint_dir);

    ~CheckpointManager() = default;

    // Non-copyable
    CheckpointManager(const CheckpointManager&) = delete;
    CheckpointManager& operator=(const CheckpointManager&) = delete;

    // ─────────────────────────────────────────────────────────────────────────
    // Core Operations
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Save checkpoint to disk
     * @param data Checkpoint data to persist
     * @param min_interval Minimum time between saves (default: 30s)
     * @return true if saved, false if skipped due to interval
     * @note Thread-safe with internal mutex
     */
    bool save(const CheckpointData& data,
              std::chrono::seconds min_interval = std::chrono::seconds{30});

    /**
     * @brief Force save checkpoint immediately
     * @param data Checkpoint data to persist
     */
    void force_save(const CheckpointData& data);

    /**
     * @brief Load existing checkpoint if available and valid
     * @param task_id Task identifier to look up
     * @param config_hash Expected config hash for consistency check
     * @return Checkpoint data if exists and valid, nullopt otherwise
     */
    [[nodiscard]] std::optional<CheckpointData> load(const std::string& task_id,
                                                     const std::string& config_hash = "");

    /**
     * @brief Delete checkpoint file after successful completion
     * @param task_id Task identifier to clean up
     */
    void cleanup(const std::string& task_id);

    /**
     * @brief Check if a checkpoint exists for given task
     */
    [[nodiscard]] bool exists(const std::string& task_id) const;

    // ─────────────────────────────────────────────────────────────────────────
    // Utility
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Get checkpoint file path for a task
     */
    [[nodiscard]] std::filesystem::path get_checkpoint_path(const std::string& task_id) const;

    /**
     * @brief Calculate progress percentage from checkpoint
     */
    [[nodiscard]] static double calculate_progress(const CheckpointData& data);

private:
    std::filesystem::path m_checkpoint_dir;
    std::chrono::steady_clock::time_point m_last_save_time;
    std::mutex m_mutex;

    /**
     * @brief Serialize checkpoint to JSON string
     */
    [[nodiscard]] std::string serialize(const CheckpointData& data) const;

    /**
     * @brief Deserialize JSON string to checkpoint
     */
    [[nodiscard]] std::optional<CheckpointData> deserialize(const std::string& json) const;

    /**
     * @brief Verify checkpoint integrity using checksum
     */
    [[nodiscard]] bool verify_integrity(const CheckpointData& data) const;

    /**
     * @brief Calculate checksum for checkpoint data
     */
    [[nodiscard]] std::string calculate_checksum(const CheckpointData& data) const;

    /**
     * @brief Helper for atomic save
     */
    void force_save_internal(const CheckpointData& data);

    /**
     * @brief Get current ISO timestamp
     */
    [[nodiscard]] static std::string get_iso_timestamp();
};

} // namespace services::pipeline
