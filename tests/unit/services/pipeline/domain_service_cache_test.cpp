#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <functional>

import services.pipeline.runner;

using namespace services::pipeline;

// DomainServiceCache: 按 key 缓存 domain 服务实例（T3：多视频任务复用，避免每视频重建+ONNX 重解析）

TEST(DomainServiceCacheTest, GetOrCreateReturnsSameInstanceForKey) {
    DomainServiceCache cache;
    int factory_calls = 0;
    auto factory = [&]() -> std::shared_ptr<void> {
        factory_calls++;
        return std::make_shared<int>(42);
    };

    auto a = cache.get_or_create("swapper:inswapper_128", factory);
    auto b = cache.get_or_create("swapper:inswapper_128", factory);

    ASSERT_NE(a, nullptr);
    ASSERT_EQ(a, b);
    EXPECT_EQ(factory_calls, 1);
    EXPECT_EQ(*std::static_pointer_cast<int>(a), 42);
}

TEST(DomainServiceCacheTest, FactoryInvokedOncePerKey) {
    DomainServiceCache cache;
    int factory_calls = 0;
    auto factory = [&]() -> std::shared_ptr<void> {
        factory_calls++;
        return std::make_shared<std::string>("svc");
    };

    for (int i = 0; i < 5; ++i) { cache.get_or_create("enhancer:gfpgan_1.4", factory); }

    EXPECT_EQ(factory_calls, 1);
    EXPECT_EQ(cache.size(), 1u);
}

TEST(DomainServiceCacheTest, DifferentKeysCreateSeparateInstances) {
    DomainServiceCache cache;
    int factory_calls = 0;
    auto factory = [&]() -> std::shared_ptr<void> {
        factory_calls++;
        return std::make_shared<int>(factory_calls);
    };

    auto a = cache.get_or_create("swapper:inswapper_128", factory);
    auto b = cache.get_or_create("enhancer:gfpgan_1.4", factory);
    auto c = cache.get_or_create("restorer:live_portrait", factory);

    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_EQ(factory_calls, 3);
    EXPECT_EQ(cache.size(), 3u);
}

TEST(DomainServiceCacheTest, ClearResetsCache) {
    DomainServiceCache cache;
    int factory_calls = 0;
    auto factory = [&]() -> std::shared_ptr<void> {
        factory_calls++;
        return std::make_shared<int>(1);
    };

    cache.get_or_create("key1", factory);
    cache.get_or_create("key2", factory);
    EXPECT_EQ(cache.size(), 2u);

    cache.clear();
    EXPECT_EQ(cache.size(), 0u);

    // 清理后重新创建
    cache.get_or_create("key1", factory);
    EXPECT_EQ(factory_calls, 3);
    EXPECT_EQ(cache.size(), 1u);
}