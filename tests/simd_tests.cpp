#include <gtest/gtest.h>
#include <benchmark/benchmark.h>
#include "SIMD.h"
#include <vector>
#include <cmath>
#include <random>

static const uint32_t TEST_ARRAY_SIZE = 10000;

// Define macros for different types of SIMD tests to reduce boilerplate code

// Macro for integer SIMD tests
#define TEST_SIMD_INTEGER_OPERATION(SIMD_TYPE, ELEMENT_TYPE, WIDTH, OPERATION, OP_NAME, VALUE_RANGE) \
TEST(SIMDTest, SIMD_TYPE##WIDTH##_##OP_NAME) \
{ \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>, TEST_ARRAY_SIZE> simd_array; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>, TEST_ARRAY_SIZE> simd_array_2; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>, TEST_ARRAY_SIZE> simd_array_original; \
    std::vector<ELEMENT_TYPE> plain_array(TEST_ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount); \
    std::vector<ELEMENT_TYPE> plain_array_2(TEST_ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount); \
    std::vector<ELEMENT_TYPE> plain_array_original(TEST_ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount); \
    \
    std::mt19937 rng(42); \
    std::uniform_int_distribution<ELEMENT_TYPE> dist(0, VALUE_RANGE); \
    for (int i = 0; i < TEST_ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount; j++) { \
            ELEMENT_TYPE value = dist(rng); \
            while(value == 0) { \
                value = dist(rng); \
            } \
            simd_array[i][j] = value; \
            simd_array_original[i][j] = value; \
            plain_array[i * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount + j] = value; \
            plain_array_original[i * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount + j] = value; \
            \
            ELEMENT_TYPE value2 = dist(rng); \
            while(value2 == 0) { \
                value2 = dist(rng); \
            } \
            simd_array_2[i][j] = value2; \
            plain_array_2[i * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount + j] = value2; \
        } \
    } \
    \
    /* SIMD operation */ \
    simd_array OPERATION simd_array_2; \
    \
    /* Plain operation */ \
    for (size_t i = 0; i < plain_array.size(); i++) { \
        plain_array[i] OPERATION plain_array_2[i]; \
    } \
    \
    /* Compare results */ \
    for (int i = 0; i < TEST_ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount; j++) { \
            ELEMENT_TYPE simdResult = simd_array[i][j]; \
            ELEMENT_TYPE plainResult = plain_array[i * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount + j]; \
            EXPECT_EQ(simdResult, plainResult); \
        } \
    } \
}

// Macro for floating-point SIMD tests
#define TEST_SIMD_FLOAT_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME) \
TEST(SIMDTest, SIMD_TYPE##WIDTH##_##OP_NAME) \
{ \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH, TEST_ARRAY_SIZE> simd_array; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH, TEST_ARRAY_SIZE> simd_array_2; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH, TEST_ARRAY_SIZE> simd_array_original; \
    std::vector<SIMD_TYPE> plain_array(TEST_ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH::ElementCount); \
    std::vector<SIMD_TYPE> plain_array_2(TEST_ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH::ElementCount); \
    std::vector<SIMD_TYPE> plain_array_original(TEST_ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH::ElementCount); \
    \
    std::mt19937 rng(42); \
    std::uniform_real_distribution<SIMD_TYPE> dist(0.0f, 1.0f); \
    for (int i = 0; i < TEST_ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH::ElementCount; j++) { \
            SIMD_TYPE value = dist(rng); \
            simd_array[i][j] = value; \
            simd_array_original[i][j] = value; \
            plain_array[i * SIMD::SIMD_TYPE##_##WIDTH::ElementCount + j] = value; \
            plain_array_original[i * SIMD::SIMD_TYPE##_##WIDTH::ElementCount + j] = value; \
            \
            SIMD_TYPE value2 = dist(rng); \
            while(value2 == 0) { \
                value2 = dist(rng); \
            } \
            simd_array_2[i][j] = value2; \
            plain_array_2[i * SIMD::SIMD_TYPE##_##WIDTH::ElementCount + j] = value2; \
        } \
    } \
    \
    /* SIMD operation */ \
    simd_array OPERATION simd_array_2; \
    \
    /* Plain operation */ \
    for (size_t i = 0; i < plain_array.size(); i++) { \
        plain_array[i] OPERATION plain_array_2[i]; \
    } \
    \
    /* Compare results */ \
    for (int i = 0; i < TEST_ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH::ElementCount; j++) { \
            SIMD_TYPE simdResult = simd_array[i][j]; \
            SIMD_TYPE plainResult = plain_array[i * SIMD::SIMD_TYPE##_##WIDTH::ElementCount + j]; \
            EXPECT_FLOAT_EQ(simdResult, plainResult); \
        } \
    } \
}

// Benchmark macros to reduce boilerplate code in benchmark functions
#define BENCHMARK_SIMD_FLOAT_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
static void BM_SIMD_##SIMD_TYPE##WIDTH##_##OP_NAME##_##ARRAY_SIZE(benchmark::State& state) { \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH, ARRAY_SIZE> simd_array; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH, ARRAY_SIZE> simd_array_2; \
    std::mt19937 rng(42); \
    std::uniform_real_distribution<SIMD_TYPE> dist(0.0f, 1.0f); \
    for (int i = 0; i < ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH::ElementCount; j++) { \
            SIMD_TYPE value = dist(rng); \
            simd_array[i][j] = value; \
            SIMD_TYPE value2 = dist(rng); \
            simd_array_2[i][j] = value2; \
        } \
    } \
    for (auto _ : state) { \
        simd_array OPERATION simd_array_2; \
        benchmark::DoNotOptimize(simd_array); \
    } \
}

#define BENCHMARK_PLAIN_FLOAT_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
static void BM_Plain_##SIMD_TYPE##WIDTH##_##OP_NAME##_##ARRAY_SIZE(benchmark::State& state) { \
    std::vector<SIMD_TYPE> plain_array(ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH::ElementCount); \
    std::vector<SIMD_TYPE> plain_array_2(ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH::ElementCount); \
    std::mt19937 rng(42); \
    std::uniform_real_distribution<SIMD_TYPE> dist(0.0f, 1.0f); \
    for (int i = 0; i < ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH::ElementCount; j++) { \
            SIMD_TYPE value = dist(rng); \
            plain_array[i * SIMD::SIMD_TYPE##_##WIDTH::ElementCount + j] = value; \
            SIMD_TYPE value2 = dist(rng); \
            plain_array_2[i * SIMD::SIMD_TYPE##_##WIDTH::ElementCount + j] = value2; \
        } \
    } \
    for (auto _ : state) { \
        for (size_t i = 0; i < plain_array.size(); i++) { \
            plain_array[i] OPERATION plain_array_2[i]; \
        } \
        benchmark::DoNotOptimize(plain_array); \
    } \
}

// Register benchmark with unit and name
#define REGISTER_FLOAT_BENCHMARKS(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
BENCHMARK_SIMD_FLOAT_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
BENCHMARK(BM_SIMD_##SIMD_TYPE##WIDTH##_##OP_NAME##_##ARRAY_SIZE)->Unit(benchmark::kMillisecond); \
BENCHMARK_PLAIN_FLOAT_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
BENCHMARK(BM_Plain_##SIMD_TYPE##WIDTH##_##OP_NAME##_##ARRAY_SIZE)->Unit(benchmark::kMillisecond);

// Benchmark macros for integer operations
#define BENCHMARK_SIMD_INT_OPERATION(SIMD_TYPE, ELEMENT_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE, VALUE_RANGE) \
static void BM_SIMD_##SIMD_TYPE##WIDTH##_with_##ELEMENT_TYPE##_##OP_NAME##_##ARRAY_SIZE(benchmark::State& state) { \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>, ARRAY_SIZE> simd_array; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>, ARRAY_SIZE> simd_array_2; \
    std::mt19937 rng(42); \
    std::uniform_int_distribution<ELEMENT_TYPE> dist(-VALUE_RANGE, VALUE_RANGE); \
    for (int i = 0; i < ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount; j++) { \
            ELEMENT_TYPE value = dist(rng); \
            simd_array[i][j] = value; \
            ELEMENT_TYPE value2 = dist(rng); \
            simd_array_2[i][j] = value2; \
        } \
    } \
    for (auto _ : state) { \
        simd_array OPERATION simd_array_2; \
        benchmark::DoNotOptimize(simd_array); \
        benchmark::DoNotOptimize(simd_array_2); \
    } \
}

#define BENCHMARK_PLAIN_INT_OPERATION(SIMD_TYPE, ELEMENT_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE, VALUE_RANGE) \
static void BM_Plain_##SIMD_TYPE##WIDTH##_with_##ELEMENT_TYPE##_##OP_NAME##_##ARRAY_SIZE(benchmark::State& state) { \
    std::vector<ELEMENT_TYPE> plain_array(ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount); \
    std::vector<ELEMENT_TYPE> plain_array_2(ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount); \
    std::mt19937 rng(42); \
    std::uniform_int_distribution<ELEMENT_TYPE> dist(-VALUE_RANGE, VALUE_RANGE); \
    for (int i = 0; i < ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount; j++) { \
            ELEMENT_TYPE value = dist(rng); \
            plain_array[i * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount + j] = value; \
            ELEMENT_TYPE value2 = dist(rng); \
            plain_array_2[i * SIMD::SIMD_TYPE##_##WIDTH<ELEMENT_TYPE>::ElementCount + j] = value2; \
        } \
    } \
    for (auto _ : state) { \
        for (size_t i = 0; i < plain_array.size(); i++) { \
            plain_array[i] OPERATION plain_array_2[i]; \
        } \
        benchmark::DoNotOptimize(plain_array); \
    } \
}

// Register benchmark with unit and name for integer operations
#define REGISTER_INT_BENCHMARKS(SIMD_TYPE, ELEMENT_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE, VALUE_RANGE) \
BENCHMARK_SIMD_INT_OPERATION(SIMD_TYPE, ELEMENT_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE, VALUE_RANGE) \
BENCHMARK(BM_SIMD_##SIMD_TYPE##WIDTH##_with_##ELEMENT_TYPE##_##OP_NAME##_##ARRAY_SIZE)->Unit(benchmark::kMillisecond); \
BENCHMARK_PLAIN_INT_OPERATION(SIMD_TYPE, ELEMENT_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE, VALUE_RANGE) \
BENCHMARK(BM_Plain_##SIMD_TYPE##WIDTH##_with_##ELEMENT_TYPE##_##OP_NAME##_##ARRAY_SIZE)->Unit(benchmark::kMillisecond);

// Special benchmark macros for int8_t operations
#define BENCHMARK_SIMD_INT8_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
static void BM_SIMD_##SIMD_TYPE##WIDTH##_with_int8_t_##OP_NAME##_##ARRAY_SIZE(benchmark::State& state) { \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<int8_t>, ARRAY_SIZE> simd_array; \
    SIMD::Array<SIMD::SIMD_TYPE##_##WIDTH<int8_t>, ARRAY_SIZE> simd_array_2; \
    std::mt19937 rng(42); \
    std::uniform_int_distribution<int> dist(-126, 127); /* Use int distribution and cast to int8_t */ \
    for (int i = 0; i < ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH<int8_t>::ElementCount; j++) { \
            int8_t value = static_cast<int8_t>(dist(rng)); \
            simd_array[i][j] = value; \
            int8_t value2 = static_cast<int8_t>(dist(rng)); \
            simd_array_2[i][j] = value2; \
        } \
    } \
    for (auto _ : state) { \
        simd_array OPERATION simd_array_2; \
        benchmark::DoNotOptimize(simd_array); \
    } \
}

#define BENCHMARK_PLAIN_INT8_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
static void BM_Plain_##SIMD_TYPE##WIDTH##_with_int8_t_##OP_NAME##_##ARRAY_SIZE(benchmark::State& state) { \
    std::vector<int8_t> plain_array(ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<int8_t>::ElementCount); \
    std::vector<int8_t> plain_array_2(ARRAY_SIZE * SIMD::SIMD_TYPE##_##WIDTH<int8_t>::ElementCount); \
    std::mt19937 rng(42); \
    std::uniform_int_distribution<int> dist(-126, 127); /* Use int distribution and cast to int8_t */ \
    for (int i = 0; i < ARRAY_SIZE; i++) { \
        for (int j = 0; j < SIMD::SIMD_TYPE##_##WIDTH<int8_t>::ElementCount; j++) { \
            int8_t value = static_cast<int8_t>(dist(rng)); \
            plain_array[i * SIMD::SIMD_TYPE##_##WIDTH<int8_t>::ElementCount + j] = value; \
            int8_t value2 = static_cast<int8_t>(dist(rng)); \
            plain_array_2[i * SIMD::SIMD_TYPE##_##WIDTH<int8_t>::ElementCount + j] = value2; \
        } \
    } \
    for (auto _ : state) { \
        for (size_t i = 0; i < plain_array.size(); i++) { \
            plain_array[i] OPERATION plain_array_2[i]; \
        } \
        benchmark::DoNotOptimize(plain_array); \
    } \
}

// Register benchmark with unit and name for int8_t operations
#define REGISTER_INT8_BENCHMARKS(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
BENCHMARK_SIMD_INT8_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
BENCHMARK(BM_SIMD_##SIMD_TYPE##WIDTH##_with_int8_t_##OP_NAME##_##ARRAY_SIZE)->Unit(benchmark::kMillisecond); \
BENCHMARK_PLAIN_INT8_OPERATION(SIMD_TYPE, WIDTH, OPERATION, OP_NAME, ARRAY_SIZE) \
BENCHMARK(BM_Plain_##SIMD_TYPE##WIDTH##_with_int8_t_##OP_NAME##_##ARRAY_SIZE)->Unit(benchmark::kMillisecond);

// Use the macros to define all the required tests
// Integer tests - Int128
TEST_SIMD_INTEGER_OPERATION(int, int32_t, 128, +=, Addition, 1000)
TEST_SIMD_INTEGER_OPERATION(int, int32_t, 128, -=, Subtraction, 1000)
TEST_SIMD_INTEGER_OPERATION(int, int32_t, 128, *=, Multiplication, 50)
#if defined(SVML_COMPATIBLE_COMPILER)
    TEST_SIMD_INTEGER_OPERATION(int, int32_t, 128, /=, DivisionSVML_Only, 50)
#endif
// Integer tests - Int256
TEST_SIMD_INTEGER_OPERATION(int, int32_t, 256, +=, Addition, 1000)
TEST_SIMD_INTEGER_OPERATION(int, int32_t, 256, -=, Subtraction, 1000)
TEST_SIMD_INTEGER_OPERATION(int, int32_t, 256, *=, Multiplication, 50)
#if defined(SVML_COMPATIBLE_COMPILER)
    TEST_SIMD_INTEGER_OPERATION(int, int32_t, 256, /=, DivisionSVML_Only, 50)
#endif
// Float tests - Float256
TEST_SIMD_FLOAT_OPERATION(float, 256, +=, Addition)
TEST_SIMD_FLOAT_OPERATION(float, 256, -=, Subtraction)
TEST_SIMD_FLOAT_OPERATION(float, 256, *=, Multiplication)
TEST_SIMD_FLOAT_OPERATION(float, 256, /=, Division)

// Define benchmarks using the macros
// Float benchmarks
REGISTER_FLOAT_BENCHMARKS(float, 256, +=, Addition, 100000)
REGISTER_FLOAT_BENCHMARKS(float, 256, -=, Subtraction, 100000)
REGISTER_FLOAT_BENCHMARKS(float, 256, *=, Multiplication, 100000)
REGISTER_FLOAT_BENCHMARKS(float, 256, /=, Division, 100000)

// Double benchmarks
REGISTER_FLOAT_BENCHMARKS(double, 256, +=, Addition, 100000)
REGISTER_FLOAT_BENCHMARKS(double, 256, -=, Subtraction, 100000)
REGISTER_FLOAT_BENCHMARKS(double, 256, *=, Multiplication, 100000)
REGISTER_FLOAT_BENCHMARKS(double, 256, /=, Division, 100000)

// Int128 benchmarks
REGISTER_INT_BENCHMARKS(int, int32_t, 128, +=, Addition, 1000000, 1000)
REGISTER_INT_BENCHMARKS(int, int32_t, 128, -=, Subtraction, 1000000, 1000)
REGISTER_INT_BENCHMARKS(int, int32_t, 128, *=, Multiplication, 100000, 50)

REGISTER_INT_BENCHMARKS(int, int16_t, 128, +=, Addition, 100000, 1000)
REGISTER_INT_BENCHMARKS(int, int16_t, 128, -=, Subtraction, 100000, 1000)
REGISTER_INT_BENCHMARKS(int, int16_t, 128, *=, Multiplication, 100000, 50)

// Int128 benchmarks with int8_t
REGISTER_INT8_BENCHMARKS(int, 128, +=, Addition, 100000)
REGISTER_INT8_BENCHMARKS(int, 128, -=, Subtraction, 100000)

// Int256 benchmarks
REGISTER_INT_BENCHMARKS(int, int32_t, 256, +=, Addition, 100000, 1000)
REGISTER_INT_BENCHMARKS(int, int32_t, 256, -=, Subtraction, 100000, 1000)
REGISTER_INT_BENCHMARKS(int, int32_t, 256, *=, Multiplication, 100000, 50)
REGISTER_INT_BENCHMARKS(int, int16_t, 256, +=, Addition, 100000, 1000)
REGISTER_INT_BENCHMARKS(int, int16_t, 256, -=, Subtraction, 100000, 1000)
REGISTER_INT_BENCHMARKS(int, int16_t, 256, *=, Multiplication, 100000, 50)

// Int256 benchmarks with int8_t
REGISTER_INT8_BENCHMARKS(int, 256, +=, Addition, 100000)
REGISTER_INT8_BENCHMARKS(int, 256, -=, Subtraction, 100000)

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int test_result = RUN_ALL_TESTS();
    if (test_result != 0) return test_result;
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    return 0;
}