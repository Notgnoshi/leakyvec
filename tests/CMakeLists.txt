find_package(GTest REQUIRED)

add_executable(leakyvec-tests test_placeholder.cpp)
target_link_libraries(leakyvec-tests PUBLIC leakyvec)
target_link_libraries(leakyvec-tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(leakyvec-tests)
