cmake_minimum_required(VERSION 3.10)

# Clone GoogleTest if not present
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/googletest")
  execute_process(
    COMMAND git clone https://github.com/google/googletest.git "${CMAKE_SOURCE_DIR}/third_party/googletest"
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to clone googletest")
  endif()
endif()

# Clone Google Benchmark if not present
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/benchmark")
  execute_process(
    COMMAND git clone https://github.com/google/benchmark.git "${CMAKE_SOURCE_DIR}/third_party/benchmark"
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to clone benchmark")
  endif()
endif()


project(BasicSIMD)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

add_library(BasicSIMD INTERFACE)
target_include_directories(BasicSIMD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(BasicSIMD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/SIMD.h)

if(MSVC)
    target_compile_options(BasicSIMD INTERFACE 
        /arch:AVX2 
    )
else()
    target_compile_options(BasicSIMD INTERFACE -march:native -fno-strict-aliasing)
endif()

# Tests
if (NOT DISABLE_TESTS)

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(gtest_build_samples OFF CACHE BOOL "" FORCE)
    set(gtest_build_tests OFF CACHE BOOL "" FORCE)
    set(gmock_build_tests OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest)
    # Add Google Benchmark
    add_subdirectory(third_party/benchmark)

    # Your test executable
    add_executable(BasicSIMD_Tests tests/simd_tests.cpp)
    target_link_libraries(BasicSIMD_Tests PRIVATE BasicSIMD gtest gtest_main benchmark benchmark_main)

endif()