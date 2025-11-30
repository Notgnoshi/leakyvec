#include "log-allocator.hpp"
#include "mock-allocator.hpp"

#include <leakyvec/leakyvec.hpp>

#include <gmock/gmock.h>

TEST(VecWrapperTests, uint8_t)
{
    auto v = std::vector<uint8_t, testing::LogAllocator<uint8_t>>{1, 2, 3, 4};
    v.reserve(10);

    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);

    leaky::detail::VecWrapper<uint8_t, testing::LogAllocator<uint8_t>> wrapper{std::move(v)};

    const auto* data_start = wrapper.get_data_start();
    ASSERT_EQ(data_start, wrapper.inner.data());

    const auto* one_past_data_end = wrapper.get_data_end();
    const auto* data_end = one_past_data_end - 1;
    ASSERT_EQ(data_end[0], 4);
    ASSERT_EQ(one_past_data_end - data_start, 4);

    const auto* capacity_end = wrapper.get_capacity_end();
    ASSERT_EQ(capacity_end - data_start, 10);
}

TEST(VecWrapperTests, uint64_t)
{
    auto v = std::vector<uint64_t>{1, 2, 3, 4};
    v.reserve(10);

    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);

    leaky::detail::VecWrapper<uint64_t> wrapper{std::move(v)};

    const auto* data_start = wrapper.get_data_start();
    ASSERT_EQ(data_start, wrapper.inner.data());

    const auto* one_past_data_end = wrapper.get_data_end();
    const auto* data_end = one_past_data_end - 1;
    ASSERT_EQ(data_end[0], 4);
    ASSERT_EQ(one_past_data_end - data_start, 4);

    const auto* capacity_end = wrapper.get_capacity_end();
    ASSERT_EQ(capacity_end - data_start, 10);
}
