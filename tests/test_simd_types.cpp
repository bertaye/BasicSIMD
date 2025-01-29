#include <gtest/gtest.h>
#include "../SIMD.h"
#include <string>


//Should be CONSTRUCTIBLE
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int, int, int, int, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int, int, int, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int, int, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int16_t>>);


static_assert(std::is_constructible_v<SIMD::int_128<int32_t>, int, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int32_t>, int, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int32_t>, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int32_t>, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int32_t>>);

static_assert(std::is_constructible_v<SIMD::int_128<int64_t>, int, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int64_t>, int>);
static_assert(std::is_constructible_v<SIMD::int_128<int64_t>>);

static_assert(std::is_constructible_v<SIMD::float_512, float, float, float, float, float, float, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float, float, float, float, float, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float, float, float, float, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float, float, float, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float, float, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float, float>);
static_assert(std::is_constructible_v<SIMD::float_512, float>);
static_assert(std::is_constructible_v<SIMD::float_512>);

static_assert(std::is_constructible_v<SIMD::double_512, double, double>);
static_assert(std::is_constructible_v<SIMD::double_512, double>);
static_assert(std::is_constructible_v<SIMD::double_512>);

TEST(SIMD_Test, SIMD_BasicOperators) 
{
    SIMD::int_128<int> test_1(1, 2, 3, 4);
    SIMD::int_128<int> test_2(5, 6, 7, 8);
    SIMD::int_128<int> test_3 = test_1 + test_2;

    for(int i=0; i< test_3.ElementCount; i++)
    {
        EXPECT_EQ(test_3[i], test_1[i] + test_2[i]);
    }
}

TEST(SIMD_Test, SIMD_Import) 
{
    void* data;
    int res = allocate_aligned(data, SIMD::int_256<int16_t>::SizeBytes, SIMD::int_256<int16_t>::Alignment);
    EXPECT_EQ(res, 0);
    SIMD::int_256<int16_t> test = SIMD::int_256<int16_t>::Import(data);

    for(int i=0; i< test.ElementCount; i++)
    {
        *((int16_t*)data + i) = i;
    }

    for(int i=0; i< test.ElementCount; i++)
    {
        EXPECT_EQ(test[i], i);
    }
}