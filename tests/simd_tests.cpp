#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "SIMD.h"

TEST_CASE("Addition Operator", "[SIMD]")
{
	SIMD::int_128<int> test_1(1, 2, 3, 4);
	SIMD::int_128<int> test_2(5, 6, 7, 8);
	SIMD::int_128<int> test_3 = test_1 + test_2;

	for (int i = 0; i < test_3.ElementCount; i++)
	{
		REQUIRE(test_3[i] == test_1[i] + test_2[i]);
	}
}

TEST_CASE("Import", "[SIMD]")
{
    void* data;
    int res = allocate_aligned(data, SIMD::int_256<int16_t>::SizeBytes, SIMD::int_256<int16_t>::Alignment);
    REQUIRE(res == 0);
    SIMD::int_256<int16_t> test = SIMD::int_256<int16_t>::Import(data);

    for(int i=0; i< test.ElementCount; i++)
    {
        *((int16_t*)data + i) = i;
    }

    for(int i=0; i< test.ElementCount; i++)
    {
        REQUIRE(test[i] == i);
    }

		// Replace std::array with std::vector for heap allocation
	SIMD::Array<SIMD::int_256<int16_t>, 15000> testArray;
	std::vector<int16_t> plainArray(testArray.Length * SIMD::int_256<int16_t>::ElementCount);

	SIMD::Array<SIMD::int_256<int16_t>, 15000> testArray2;
	std::vector<int16_t> plainArray2(testArray2.Length * SIMD::int_256<int16_t>::ElementCount);

	// Initialize testArray and plainArray
	for(int i = 0; i < testArray.Length; i++)
	{
		for(int j = 0; j < testArray[i].ElementCount; j++)
		{
			testArray[i][j] = j;
			plainArray[i * SIMD::int_256<int16_t>::ElementCount + j] = j;
		}
	}

	// Initialize testArray2 and plainArray2
	for (int i = 0; i < testArray2.Length; i++)
	{
		for (int j = 0; j < testArray2[i].ElementCount; j++)
		{
			testArray2[i][j] = j;
			plainArray2[i * SIMD::int_256<int16_t>::ElementCount + j] = j;
		}
	}

	BENCHMARK("SIMD Array Addition")
	{
		testArray += testArray2;
	};

	BENCHMARK("Plain Array Addition")
	{
		for (int i = 0; i < plainArray.size(); i++)
		{
			plainArray[i] += plainArray2[i];
		}
	};
}