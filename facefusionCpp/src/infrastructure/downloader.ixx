/**
 ******************************************************************************
 * @file           : downloader.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-13
 ******************************************************************************
 */

module;
#include <curl/curl.h>
#include <string>
#include <unordered_set>
#include "common_macros.h" // don't remove

export module downloader;

namespace ffc::downloader {

export bool download(const std::string& url, const std::string& outPutDirectory);
export std::vector<bool> batch_download(const std::vector<std::string>& urls, const std::string& output_dir_path);
export bool is_downloaded(const std::string& url, const std::string& file_path);
export std::string get_file_name_from_url(const std::string& url);
export long get_file_size_from_url(const std::string& url);
// 写入数据的回调函数
size_t write_data(void* ptr, size_t size, size_t nmemb, std::ofstream* stream);
// 进度回调函数
int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
// 将数据丢弃的函数
size_t empty_write_fn(void* ptr, size_t size, size_t nmemb, void* userdata);
std::string human_readable_size(const long& size);
size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
bool is_url_valid(const std::string& url);
} // namespace ffc::downloader
