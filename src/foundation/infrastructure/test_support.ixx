module;
#include <filesystem>
#include <string>
export module foundation.infrastructure.test_support;

export import foundation.infrastructure.core_utils;
export import foundation.infrastructure.network;
export import foundation.infrastructure.file_system;
export import foundation.infrastructure.concurrent_file_system;
export import foundation.infrastructure.crypto;
export import foundation.infrastructure.concurrent_crypto;
export import foundation.infrastructure.logger;
export import foundation.infrastructure.progress;
export import foundation.infrastructure.thread_pool;

export namespace foundation::infrastructure::test {
void reset_environment();

// 获取 assets 目录的绝对路径
// 搜索顺序: 环境变量 FACEFUSION_ASSETS_PATH -> 当前目录 -> 父目录递归
std::filesystem::path get_assets_path();

// 获取特定测试数据的绝对路径
// relative_path: 相对于 assets 目录的路径
std::filesystem::path get_test_data_path(const std::string& relative_path);
} // namespace foundation::infrastructure::test
