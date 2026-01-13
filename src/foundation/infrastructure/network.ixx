module;
#include <string>
#include <vector>

export module foundation.infrastructure.network;

// import <string>;
// import <vector>;

export namespace foundation::infrastructure::network {
bool download(const std::string& url, const std::string& output_dir);
std::vector<bool> batch_download(const std::vector<std::string>& urls,
                                 const std::string& output_dir);
bool is_downloaded(const std::string& url, const std::string& file_path);
long get_file_size_from_url(const std::string& url);
std::string get_file_name_from_url(const std::string& url);
std::string human_readable_size(long size);
} // namespace foundation::infrastructure::network
