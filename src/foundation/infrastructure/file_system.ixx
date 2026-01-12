module;
#include <string>
#include <vector>

export module foundation.infrastructure.file_system;

export namespace foundation::infrastructure::file_system {
/**
 * Remove a file at the specified path.
 * @param path The path to the file to remove.
 */
void remove_file(const std::string& path);

/**
 * Remove multiple files.
 * @param paths A vector of paths to files to remove.
 */
void remove_files(const std::vector<std::string>& paths);

/**
 * Copy a file from source to destination.
 * @param source The source file path.
 * @param destination The destination file path.
 */
void copy_file(const std::string& source, const std::string& destination);

/**
 * Alias for copy_file.
 */
inline void copy(const std::string& source, const std::string& destination) {
    copy_file(source, destination);
}

/**
 * Copy multiple files to a destination directory.
 * @param sources A vector of source file paths.
 * @param destination The destination directory path.
 */
void copy_files(const std::vector<std::string>& sources, const std::string& destination);

/**
 * Check if a file exists.
 * @param path The path to check.
 * @return True if the file exists, false otherwise.
 */
[[nodiscard]] bool file_exists(const std::string& path);

/**
 * Check if a directory exists.
 * @param path The path to check.
 * @return True if the directory exists, false otherwise.
 */
[[nodiscard]] bool dir_exists(const std::string& path);

/**
 * Check if the path refers to a regular file.
 * @param path The path to check.
 * @return True if it is a regular file, false otherwise.
 */
[[nodiscard]] bool is_file(const std::string& path);

/**
 * Check if the path refers to a directory.
 * @param path The path to check.
 * @return True if it is a directory, false otherwise.
 */
[[nodiscard]] bool is_dir(const std::string& path);

/**
 * Create directories recursively.
 * @param path The path to create.
 */
void create_directories(const std::string& path);

/**
 * Alias for create_directories.
 */
inline void create_dir(const std::string& path) {
    create_directories(path);
}

/**
 * Get the parent path of a given path.
 * @param path The path to query.
 * @return The parent path.
 */
[[nodiscard]] std::string parent_path(const std::string& path);

/**
 * Get the base name (stem) of a file path.
 * @param path The path to query.
 * @return The stem of the file.
 */
[[nodiscard]] std::string get_base_name(const std::string& path);

/**
 * Get the file name of a file path.
 * @param path The path to query.
 * @return The file name.
 */
[[nodiscard]] std::string get_file_name(const std::string& path);

/**
 * Get the file extension of a file path.
 * @param path The path to query.
 * @return The file extension.
 */
[[nodiscard]] std::string get_file_ext(const std::string& path);

/**
 * Get the absolute path.
 * @param path The input path.
 * @return The absolute path.
 */
[[nodiscard]] std::string absolute_path(const std::string& path);

/**
 * Convert UTF-8 string to system default local encoding (dummy implementation).
 * @param str The UTF-8 string.
 * @return The system default equivalent.
 */
[[nodiscard]] std::string utf8_to_sys_default_local(const std::string& str);

} // namespace foundation::infrastructure::file_system
