module;

#include <string>
#include <vector>

export module foundation.infrastructure.network;

export namespace foundation::infrastructure::network {

/**
 * @brief Download a file from URL to specified output directory
 * @param url The URL of the file to download
 * @param output_dir The directory to save the downloaded file
 * @return bool True if download succeeded, false otherwise
 * @throws std::invalid_argument if URL is empty
 * @throws std::runtime_error if output directory creation fails or download fails
 * @note Thread-safe (each download operation is independent)
 * @exception-safe Strong guarantee
 */
bool download(const std::string& url, const std::string& output_dir);

/**
 * @brief Download multiple files from URLs to specified output directory
 * @param urls Vector of URLs to download
 * @param output_dir The directory to save the downloaded files
 * @return std::vector<bool> Vector of boolean results for each URL
 * @throws std::invalid_argument if any URL is empty
 * @throws std::runtime_error if output directory creation fails
 * @note Thread-safe (each download operation is independent)
 * @exception-safe Strong guarantee
 */
std::vector<bool> batch_download(const std::vector<std::string>& urls,
                                 const std::string& output_dir);

/**
 * @brief Check if a file has been downloaded and matches expected size
 * @param url The URL of the file to check
 * @param file_path The local file path to check
 * @return bool True if file exists and matches expected size, false otherwise
 * @throws std::invalid_argument if URL or file path is empty
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
bool is_downloaded(const std::string& url, const std::string& file_path);

/**
 * @brief Get file size from URL using HTTP HEAD request
 * @param url The URL to query for file size
 * @return long File size in bytes, or -1 if failed to retrieve
 * @throws std::invalid_argument if URL is empty
 * @note Thread-safe (each request is independent)
 * @exception-safe Strong guarantee
 */
long get_file_size_from_url(const std::string& url);

/**
 * @brief Extract file name from URL
 * @param url The URL to extract file name from
 * @return std::string File name extracted from URL
 * @throws std::invalid_argument if URL is empty
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
std::string get_file_name_from_url(const std::string& url);

/**
 * @brief Convert file size to human-readable format
 * @param size File size in bytes
 * @return std::string Human-readable size string (e.g., "1.5 MB")
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
std::string human_readable_size(const long& size);

} // namespace foundation::infrastructure::network
