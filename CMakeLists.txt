cmake_minimum_required(VERSION 3.22) # Ubuntu 22.04
project(
    leakyvec
    VERSION 0.1.0
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LEAKY_USE_ASAN "Enable AddressSanitizer" OFF)
option(LEAKY_USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

set(SANITIZER_FLAGS "")
if(LEAKY_USE_ASAN)
    set(SANITIZER_FLAGS "-fno-omit-frame-pointer -fsanitize=address")
elseif(LEAKY_USE_UBSAN)
    set(SANITIZER_FLAGS "-fno-omit-frame-pointer -fsanitize=undefined")
endif()

if(SANITIZER_FLAGS)
    message(STATUS "Enabling sanitizers with flags: ${SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

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
