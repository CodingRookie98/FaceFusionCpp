#include <gtest/gtest.h>

/**
 * @brief 示例测试类 - 演示 GoogleTest 的基本用法
 * @details 这个测试文件展示了如何使用 GoogleTest 编写单元测试
 */
class ExampleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 在每个测试用例运行前调用
    }

    void TearDown() override {
        // 在每个测试用例运行后调用
    }
};

/**
 * @brief 基本断言测试
 * @details 演示 EXPECT_* 和 ASSERT_* 宏的使用
 */
TEST_F(ExampleTest, BasicAssertions) {
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}

/**
 * @brief 布尔断言测试
 */
TEST_F(ExampleTest, BooleanAssertions) {
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

/**
 * @brief 数值比较测试
 */
TEST_F(ExampleTest, NumericComparisons) {
    EXPECT_EQ(1, 1);
    EXPECT_NE(1, 2);
    EXPECT_LT(1, 2);
    EXPECT_LE(1, 2);
    EXPECT_GT(2, 1);
    EXPECT_GE(2, 1);
}

/**
 * @brief 字符串比较测试
 */
TEST_F(ExampleTest, StringComparisons) {
    const char* str1 = "hello";
    const char* str2 = "world";

    EXPECT_STREQ(str1, "hello");
    EXPECT_STRNE(str1, str2);
}

/**
 * @brief 浮点数比较测试
 */
TEST_F(ExampleTest, FloatingPointComparisons) {
    EXPECT_FLOAT_EQ(1.0f, 1.0f + 0.0000001f);
    EXPECT_NEAR(1.0, 1.0 + 0.000000000001, 0.00000000001);
}

/**
 * @brief 异常测试
 */
TEST_F(ExampleTest, ExceptionTest) {
    EXPECT_THROW(throw std::runtime_error("test"), std::runtime_error);
    EXPECT_ANY_THROW(throw std::runtime_error("test"));
    EXPECT_NO_THROW(1 + 1);
}

/**
 * @brief 自定义断言示例
 */
TEST_F(ExampleTest, CustomAssertion) {
    auto is_even = [](int n) { return n % 2 == 0; };

    EXPECT_TRUE(is_even(2));
    EXPECT_FALSE(is_even(3));
}

/**
 * @brief 参数化测试示例
 */
class ParameterizedTest : public ::testing::TestWithParam<int> {
};

TEST_P(ParameterizedTest, IsEven) {
    int n = GetParam();
    EXPECT_EQ(n % 2, 0);
}

INSTANTIATE_TEST_SUITE_P(
    EvenNumbers,
    ParameterizedTest,
    ::testing::Values(2, 4, 6, 8, 10)
);

/**
 * @brief 测试夹具示例
 */
class Counter {
public:
    void increment() { ++m_count; }
    void decrement() { --m_count; }
    int get() const { return m_count; }

private:
    int m_count = 0;
};

TEST_F(ExampleTest, CounterTest) {
    Counter counter;

    EXPECT_EQ(counter.get(), 0);

    counter.increment();
    EXPECT_EQ(counter.get(), 1);

    counter.increment();
    EXPECT_EQ(counter.get(), 2);

    counter.decrement();
    EXPECT_EQ(counter.get(), 1);
}

/**
 * @brief 主函数（由 GTest::gtest_main 提供）
 * @note 不需要手动实现 main()，GTest::gtest_main 会自动提供
 */
