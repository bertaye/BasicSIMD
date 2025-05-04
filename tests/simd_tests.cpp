#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "SIMD.h"
TEST_CASE("Float Multiplication", "[SIMD]")
{
    CPUFeatures::printSupportedInstructionSets();
    // Create float SIMD arrays
    constexpr int arraySize = 1;
    SIMD::Array<SIMD::float_256, arraySize> simdArray;
    SIMD::Array<SIMD::float_256, arraySize> simdArray2;
    SIMD::Array<SIMD::float_256, arraySize> simdArray_original;
    
    // Create plain float arrays for comparison
    std::vector<float> plainArray(arraySize * SIMD::float_256::ElementCount);
    std::vector<float> plainArray2(arraySize * SIMD::float_256::ElementCount);
    std::vector<float> plainArray_original(arraySize * SIMD::float_256::ElementCount);
    // Initialize with test values
    for (int i = 0; i < arraySize; i++) {
        for (int j = 0; j < SIMD::float_256::ElementCount; j++) {
            float value = static_cast<float>(j + 1) * 0.5f;  // Use 0.5, 1.0, 1.5, etc.

            simdArray[i][j] = value;
            simdArray_original[i][j] = value;

            plainArray[i * SIMD::float_256::ElementCount + j] = value;
            plainArray_original[i * SIMD::float_256::ElementCount + j] = value;
            
            float value2 = static_cast<float>(j + 2) * 0.25f;  // Use 0.5, 0.75, 1.0, etc.
            simdArray2[i][j] = value2;
            plainArray2[i * SIMD::float_256::ElementCount + j] = value2;
        }
    }
    
    // Perform multiplication and benchmark
    BENCHMARK_ADVANCED("SIMD Float Multiplication")(Catch::Benchmark::Chronometer meter) {

        meter.measure( [&simdArray, &simdArray2]() { 
            simdArray *= simdArray2;
        });
        simdArray = simdArray_original; 
    };
    BENCHMARK_ADVANCED("Plain Float Multiplication")(Catch::Benchmark::Chronometer meter) {
        meter.measure( [&plainArray, &plainArray2]() {
        for (size_t i = 0; i < plainArray.size(); i++) {
            plainArray[i] *= plainArray2[i];
        }
        });
        for (size_t i = 0; i < plainArray.size(); i++) {
            plainArray[i] = plainArray_original[i];
        }
    };
    
    // Verify results
    for (int i = 0; i < arraySize; i++) {
        for (int j = 0; j < SIMD::float_256::ElementCount; j++) {
            float simdResult = simdArray[i][j];
            float plainResult = plainArray[i * SIMD::float_256::ElementCount + j];
            REQUIRE(std::abs(simdResult - plainResult) < 1e-6f);
        }
    }
}