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
#include "common_macros.h"// don't remove

export module downloader;

namespace ffc {

export class Downloader {
public:
    static bool download(const std::string &url, const std::string &outPutDirectory);
    static bool batchDownload(const std::unordered_set<std::string> &urls, const std::string &outPutDirectory);
    static bool isDownloadDone(const std::string &Url, const std::string &filePath);
    static std::string getFileNameFromUrl(const std::string &url);

private:
    static long getFileSize(const std::string &url);
    // 写入数据的回调函数
    static size_t writeData(void *ptr, size_t size, size_t nmemb, std::ofstream *stream);
    // 进度回调函数
    static int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    // 将数据丢弃的函数
    static size_t emptyWriteFunc(void *ptr, size_t size, size_t nmemb, void *userdata);
    static std::string humanReadableSize(const long &size);
    static size_t headerCallback(char *buffer, size_t size, size_t nitems, void *userdata);
};

} // namespace Ffc
