#include <gtest/gtest.h>
#include <string>
#include <stdexcept>

import foundation.infrastructure.network;

using namespace foundation::infrastructure::network;

TEST(NetworkTest, GetFileNameFromUrl) {
    EXPECT_EQ(get_file_name_from_url("http://example.com/file.txt"), "file.txt");
    EXPECT_EQ(get_file_name_from_url("http://example.com/path/to/image.png"), "image.png");
    
    // According to implementation, paths without '/' return default name
    EXPECT_EQ(get_file_name_from_url("file.zip"), "downloaded_file");
    
    // Query string handling
    EXPECT_EQ(get_file_name_from_url("http://example.com/file.txt?query=1"), "file.txt");
    
    // Trailing slash returns default name
    EXPECT_EQ(get_file_name_from_url("http://example.com/"), "downloaded_file");
    
    // Empty throws
    EXPECT_THROW(get_file_name_from_url(""), std::invalid_argument);
}

TEST(NetworkTest, HumanReadableSize) {
    EXPECT_EQ(human_readable_size(500), "500.00 B");
    EXPECT_EQ(human_readable_size(1024), "1.00 KB");
    EXPECT_EQ(human_readable_size(1536), "1.50 KB");
    EXPECT_EQ(human_readable_size(1024 * 1024), "1.00 MB");
    EXPECT_EQ(human_readable_size(1024 * 1024 * 1024), "1.00 GB");
}
