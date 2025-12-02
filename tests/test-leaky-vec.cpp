#include "mock-allocator.hpp"

#include <leakyvec/leakyvec.hpp>

#include <gmock/gmock.h>

/// Test that we can leak the memory from a std::vector without freeing it
TEST(LeakyVecTests, LeakAndDoNotFree)
{
    auto alloc = testing::MockAllocator<int>{};  // is a strict mock; any unexpected calls will fail

    EXPECT_CALL(*alloc.mock, allocate(4)).Times(1);                // initial allocation
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);               // allocate 10 on resize
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 4)).Times(1);  // deallocate first allocation

    // The 10-capacity allocation was leaked!
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(0);

    auto v = std::vector<int, testing::MockAllocator<int>>(4, alloc);
    v.reserve(10);
    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);
    auto leaky_v = leaky::Vec<int, testing::MockAllocator<int>>(std::move(v));

    // Intentional memory leak as a part of the test
    auto parts = leaky_v.leak();
    static_cast<void>(parts);
}

/// Test that we can leak the memory from a std::vector and then manually free it
TEST(LeakyVecTests, LeakAndManuallyFree)
{
    auto alloc = testing::MockAllocator<int>{};

    EXPECT_CALL(*alloc.mock, allocate(4)).Times(1);                // initial allocation
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);               // allocate 10 on resize
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 4)).Times(1);  // deallocate first allocation

    // The 10-capacity allocation was freed manually
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(1);

    auto v = std::vector<int, testing::MockAllocator<int>>(4, alloc);
    v.reserve(10);
    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);
    auto leaky_v = leaky::Vec<int, testing::MockAllocator<int>>(std::move(v));

    auto parts = leaky_v.leak();
    auto [data, size, capacity, allocator] = parts;

    allocator.deallocate(data, capacity);
}

/// Test that we can take apart and reconstruct a leaky Vec from its internals
TEST(LeakyVecTests, LeakAndReconstruct)
{
    auto alloc = testing::MockAllocator<int>{};

    EXPECT_CALL(*alloc.mock, allocate(4)).Times(1);                // initial allocation
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);               // allocate 10 on resize
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 4)).Times(1);  // deallocate first allocation

    // The 10-capacity allocation was freed by dropping the reconstructed vector
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(1);

    auto v = std::vector<int, testing::MockAllocator<int>>(4, alloc);
    v.reserve(10);
    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);
    auto leaky_v = leaky::Vec<int, testing::MockAllocator<int>>(std::move(v));

    auto parts = leaky_v.leak();

    auto leaky_v2 = leaky::Vec<int, testing::MockAllocator<int>>::from_parts(parts);
    auto v2 = leaky_v2.take();
}
