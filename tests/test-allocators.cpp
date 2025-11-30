#include "log-allocator.hpp"
#include "mock-allocator.hpp"

#include <leakyvec/leakyvec.hpp>

#include <gmock/gmock.h>

/// Create a std::vector with the LogAllocator and see it log allocations and deallocations
TEST(OwnershipTests, LogAllocator)
{
    auto v = std::vector<int, testing::LogAllocator<int>>(10, testing::LogAllocator<int>{});
    ASSERT_EQ(v.size(), 10);
    ASSERT_GE(v.capacity(), 10);

    v.push_back(42);

    ASSERT_EQ(v.size(), 11);
    ASSERT_GE(v.capacity(), 11);

    testing::LogAllocator<int> log_alloc;
    auto w = std::vector<int, testing::LogAllocator<int>>(10, log_alloc);
}

TEST(OwnershipTests, MockAllocator)
{
    testing::MockAllocator<int> alloc;

    // Initial allocation of 10 ints
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);
    // Allocate a new block large enough to hold 11 ints. An implementation detail of std::vector is
    // that it doubles capacity on growth to amortize cost.
    EXPECT_CALL(*alloc.mock, allocate(20)).Times(1);
    // Deallocate the original block of 10 ints
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(1);
    // Deallocate the final block of 20 ints on vector destruction
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 20)).Times(1);

    auto v = std::vector<int, testing::MockAllocator<int>>(10, alloc);
    ASSERT_EQ(v.size(), 10);

    v.push_back(42);

    ASSERT_EQ(v.size(), 11);
    ASSERT_GE(v.capacity(), 11);
}
