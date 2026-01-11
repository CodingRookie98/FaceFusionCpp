
module;
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <locale>


module foundation.infrastructure.network;

// Forward declarations if needed within the module implementation
namespace foundation::infrastructure::network {
std::string get_file_name_from_url(const std::string& url);
long get_file_size_from_url(const std::string& url);
} // namespace foundation::infrastructure::network

namespace foundation::infrastructure::network {

namespace {

/**
 * @brief RAII wrapper for CURL handle
 */
class CurlHandle {
public:
    CurlHandle() : m_curl(curl_easy_init()) {
        if (!m_curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }

    ~CurlHandle() {
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
    }

    CurlHandle(const CurlHandle&)            = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;

    CURL* get() const { return m_curl; }

private:
    CURL* m_curl;
};

/**
 * @brief Write callback function for CURL
 */
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size   = size * nmemb;
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    file->write(static_cast<char*>(contents), total_size);
    return total_size;
}

/**
 * @brief Header callback function for CURL (to get content length)
 */
size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_size           = size * nitems;
    std::string* content_length = static_cast<std::string*>(userdata);
    std::string header(buffer, total_size);

    if (header.find("Content-Length:") == 0) {
        size_t colon_pos = header.find(':');
        if (colon_pos != std::string::npos) {
            *content_length = header.substr(colon_pos + 1);
        }
    }
    return total_size;
}

} // anonymous namespace

bool download(const std::string& url, const std::string& output_dir) {
    if (url.empty()) {
        throw std::invalid_argument("URL cannot be empty");
    }

    std::filesystem::path output_dir_path(output_dir);
    if (!std::filesystem::exists(output_dir_path)) {
        if (!std::filesystem::create_directories(output_dir_path)) {
            throw std::runtime_error("Failed to create output directory: " + output_dir);
        }
    }

    std::string file_name                  = get_file_name_from_url(url);
    std::filesystem::path output_file_path = output_dir_path / file_name;

    CurlHandle curl;
    CURL* curl_handle = curl.get();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 300L);

    std::ofstream output_file(output_file_path.string(), std::ios::binary);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open output file: " + output_file_path.string());
    }

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &output_file);

    CURLcode res = curl_easy_perform(curl_handle);
    output_file.close();

    if (res != CURLE_OK) {
        std::filesystem::remove(output_file_path);
        throw std::runtime_error("Failed to download file: " + std::string(curl_easy_strerror(res)));
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        std::filesystem::remove(output_file_path);
        throw std::runtime_error("HTTP error: " + std::to_string(http_code));
    }

    return true;
}

std::vector<bool> batch_download(const std::vector<std::string>& urls, const std::string& output_dir) {
    if (urls.empty()) {
        return {};
    }

    for (const auto& url : urls) {
        if (url.empty()) {
            throw std::invalid_argument("URL cannot be empty");
        }
    }

    std::filesystem::path output_dir_path(output_dir);
    if (!std::filesystem::exists(output_dir_path)) {
        if (!std::filesystem::create_directories(output_dir_path)) {
            throw std::runtime_error("Failed to create output directory: " + output_dir);
        }
    }

    std::vector<bool> results;
    results.reserve(urls.size());

    for (const auto& url : urls) {
        try {
            results.push_back(download(url, output_dir));
        } catch (const std::exception&) {
            results.push_back(false);
        }
    }

    return results;
}

long get_file_size_from_url(const std::string& url) {
    if (url.empty()) {
        throw std::invalid_argument("URL cannot be empty");
    }

    CurlHandle curl;
    CURL* curl_handle = curl.get();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 60L);

    std::string content_length;
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &content_length);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        return -1;
    }

    if (content_length.empty()) {
        return -1;
    }

    try {
        std::string trimmed_content_length;
        for (char c : content_length) {
            if (c != ' ' && c != '\r' && c != '\n') {
                trimmed_content_length += c;
            }
        }
        return std::stol(trimmed_content_length);
    } catch (...) {
        return -1;
    }
}

bool is_downloaded(const std::string& url, const std::string& file_path) {
    if (url.empty()) {
        throw std::invalid_argument("URL cannot be empty");
    }
    if (file_path.empty()) {
        throw std::invalid_argument("File path cannot be empty");
    }

    std::filesystem::path file_path_obj(file_path);
    if (!std::filesystem::exists(file_path_obj)) {
        return false;
    }

    long remote_size = get_file_size_from_url(url);
    if (remote_size < 0) {
        return false;
    }

    long local_size = static_cast<long>(std::filesystem::file_size(file_path_obj));
    return local_size == remote_size;
}

std::string get_file_name_from_url(const std::string& url) {
    if (url.empty()) {
        throw std::invalid_argument("URL cannot be empty");
    }

    size_t query_pos              = url.find('?');
    std::string url_without_query = (query_pos != std::string::npos) ? url.substr(0, query_pos) : url;

    size_t last_slash_pos = url_without_query.find_last_of('/');
    if (last_slash_pos == std::string::npos) {
        return "downloaded_file";
    }

    std::string file_name = url_without_query.substr(last_slash_pos + 1);
    if (file_name.empty()) {
        return "downloaded_file";
    }

    return file_name;
}

std::string human_readable_size(long size) {
    if (size < 0) {
        return "0 B";
    }

    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index      = 0;
    double size_double  = static_cast<double>(size);

    while (size_double >= 1024.0 && unit_index < 4) {
        size_double /= 1024.0;
        unit_index++;
    }

    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::fixed << std::setprecision(2) << size_double << " " << units[unit_index];
    return oss.str();
}

} // namespace foundation::infrastructure::network
