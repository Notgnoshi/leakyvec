cmake_minimum_required(VERSION 3.22) # Ubuntu 22.04
project(
    leakyvec
    VERSION 0.1.0
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)

add_library(leakyvec INTERFACE)
target_include_directories(
    leakyvec INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(leakyvec INTERFACE cxx_std_17)
install(DIRECTORY include/leakyvec DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
