cmake_minimum_required(VERSION 3.10)
project(BasicSIMD)

set(CMAKE_CXX_STANDARD 20)

add_library(BasicSIMD INTERFACE)
target_include_directories(BasicSIMD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(BasicSIMD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/SIMD.h)

if (MSVC)
    target_compile_options(BasicSIMD INTERFACE /arch:AVX2 /arch:AVX512)
else()
    target_compile_options(BasicSIMD INTERFACE -march=native)
endif()

# Tests
if (NOT DISABLE_TESTS)
    add_subdirectory(third_party/catch2)
    add_executable(BasicSIMD_Tests tests/simd_tests.cpp)
    target_link_libraries(BasicSIMD_Tests PRIVATE BasicSIMD Catch2::Catch2)
    target_compile_definitions(BasicSIMD_Tests PRIVATE CATCH_CONFIG_ENABLE_BENCHMARKING)
endif()
