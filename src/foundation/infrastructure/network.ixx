module;
#include <string>
#include <vector>

/**
 * @file network.ixx
 * @brief Network utility module for downloading files
 * @author
 * CodingRookie
 * @date 2026-01-27
 */
export module foundation.infrastructure.network;

export namespace foundation::infrastructure::network {

/**
 * @brief Download a file from a URL to a directory
 * @param url URL to download from
 * @param output_dir Directory to save the file in
 * @return True if download successful, false otherwise
 */
bool download(const std::string& url, const std::string& output_dir);

/**
 * @brief Download multiple files from URLs to a directory (batch)
 * @param urls List of URLs to download
 * @param output_dir Directory to save files in
 * @return Vector of booleans indicating success status for each download
 */
std::vector<bool> batch_download(const std::vector<std::string>& urls,
                                 const std::string& output_dir);

/**
 * @brief Check if a file is already downloaded and valid
 * @param url Original URL of the file (used for validation logic if any)
 * @param file_path Path to the local file
 * @return True if file exists and is valid, false otherwise
 */
bool is_downloaded(const std::string& url, const std::string& file_path);

/**
 * @brief Get file size from URL (HEAD request)
 * @param url URL to check
 * @return File size in bytes, or -1 if failed
 */
long get_file_size_from_url(const std::string& url);

/**
 * @brief Extract file name from URL
 * @param url URL string
 * @return File name string
 */
std::string get_file_name_from_url(const std::string& url);

/**
 * @brief Convert byte size to human readable string
 * @param size Size in bytes
 * @return Human readable string (e.g. "1.5 MB")
 */
std::string human_readable_size(long size);

} // namespace foundation::infrastructure::network
