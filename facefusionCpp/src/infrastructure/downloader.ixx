/**
 * @file downloader.ixx
 * @brief Downloader module for file download operations
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides download functionality using libcurl library
 */

module;
#include <curl/curl.h>
#include <string>
#include <unordered_set>

export module downloader;

namespace ffc::infra::downloader {

/**
 * @brief Download a file from URL to specified directory
 * @param url URL to download from
 * @param outPutDirectory Directory to save the downloaded file
 * @return bool True if download was successful, false otherwise
 * @note Uses libcurl library for HTTP/HTTPS downloads
 */
export bool download(const std::string& url, const std::string& outPutDirectory);

/**
 * @brief Download multiple files in batch
 * @param urls Vector of URLs to download
 * @param output_dir_path Directory to save the downloaded files
 * @return std::vector<bool> Vector of download success status for each URL
 * @note Downloads are performed in parallel using thread pool
 */
export std::vector<bool> batch_download(const std::vector<std::string>& urls, const std::string& output_dir_path);

/**
 * @brief Check if a file has been downloaded from URL
 * @param url Source URL
 * @param file_path Local file path to check
 * @return bool True if file exists and matches URL, false otherwise
 */
export bool is_downloaded(const std::string& url, const std::string& file_path);

/**
 * @brief Extract file name from URL
 * @param url URL string
 * @return std::string File name extracted from URL
 */
export std::string get_file_name_from_url(const std::string& url);

/**
 * @brief Get file size from URL (using HEAD request)
 * @param url URL to query
 * @return long File size in bytes, or -1 if failed
 */
export long get_file_size_from_url(const std::string& url);

/**
 * @brief Write data callback function for libcurl
 * @param ptr Pointer to data to write
 * @param size Size of each data element
 * @param nmemb Number of elements
 * @param stream Output file stream
 * @return size_t Number of bytes written
 * @note This is an internal callback function used by libcurl
 */
size_t write_data(void* ptr, size_t size, size_t nmemb, std::ofstream* stream);

/**
 * @brief Progress callback function for libcurl
 * @param clientp Client data pointer
 * @param dltotal Total bytes to download
 * @param dlnow Bytes downloaded so far
 * @param ultotal Total bytes to upload
 * @param ulnow Bytes uploaded so far
 * @return int 0 to continue transfer, 1 to abort
 * @note This is an internal callback function used by libcurl
 */
int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

/**
 * @brief Empty write function to discard data
 * @param ptr Pointer to data (ignored)
 * @param size Size of each data element
 * @param nmemb Number of elements
 * @param userdata User data (ignored)
 * @return size_t Size of data (to indicate success)
 * @note This function is used for HEAD requests where response body is not needed
 */
size_t empty_write_fn(void* ptr, size_t size, size_t nmemb, void* userdata);

/**
 * @brief Convert file size to human-readable format
 * @param size File size in bytes
 * @return std::string Human-readable size string (e.g., "1.5 MB")
 */
std::string human_readable_size(const long& size);

/**
 * @brief Header callback function for libcurl
 * @param buffer Header data buffer
 * @param size Size of each header element
 * @param nitems Number of header elements
 * @param userdata User data pointer
 * @return size_t Number of bytes processed
 * @note This is an internal callback function used by libcurl
 */
size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);

/**
 * @brief Check if URL is valid
 * @param url URL string to validate
 * @return bool True if URL is valid, false otherwise
 * @note Performs a HEAD request to validate URL accessibility
 */
bool is_url_valid(const std::string& url);
} // namespace ffc::infra::downloader
