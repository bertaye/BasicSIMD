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



#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>  // For std::fixed and std::setprecision
#include <cmath>    // For std::abs
#include <tuple>

class Timer {
public:
    /**
     * @brief Constructs a Timer object and starts timing.
     * 
     * @param message Optional message to identify the timed block. Defaults to "Elapsed time".
     */
    explicit Timer(const std::string& message = "Elapsed time")
        : message_(message), 
          start_time_point_(std::chrono::high_resolution_clock::now()),
          duration_nanoseconds_(0),
          stopped_(false)
    {
    }

    /**
     * @brief Destructor that stops timing (if not already stopped) and outputs the duration in nanoseconds.
     */
    ~Timer() 
    {
        if (!stopped_) {
            stop();
        }
        std::cout << message_ << ": " << duration_nanoseconds_ << " nanoseconds" << std::endl;
    }

    /**
     * @brief Stops the timer and records the duration.
     */
    void stop()
    {
        if (!stopped_) {
            auto end_time_point = std::chrono::high_resolution_clock::now();
            auto duration = end_time_point - start_time_point_;
            duration_nanoseconds_ = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            stopped_ = true;
        }
    }

    /**
     * @brief Returns the elapsed duration in nanoseconds.
     * 
     * @return long long Elapsed time in nanoseconds.
     */
    long long getDuration() const
    {
        return duration_nanoseconds_;
    }

    /**
     * @brief Static method to compare two durations and calculate their difference and ratio.
     * 
     * @param duration1 First duration in nanoseconds.
     * @param duration2 Second duration in nanoseconds.
     * @return A tuple containing:
     *         - Difference in nanoseconds (duration1 - duration2)
     *         - Ratio (duration1 / duration2)
     *         - Indicator string describing the comparison.
     */
    static std::tuple<long long, double, std::string> compareDurations(long long duration1, long long duration2)
    {
        long long difference = duration1 - duration2;
        double ratio = (duration2 != 0) ? static_cast<double>(duration1) / static_cast<double>(duration2) : 0.0;
        std::string comparison;

        if (difference > 0) {
            // duration1 > duration2
            comparison = "First duration is longer by " + std::to_string(difference) + " nanoseconds.";
        }
        else if (difference < 0) {
            // duration2 > duration1
            difference = std::abs(difference);
            comparison = "Second duration is longer by " + std::to_string(difference) + " nanoseconds.";
        }
        else {
            // durations are equal
            comparison = "Both durations are equal.";
        }

        return std::make_tuple(difference, ratio, comparison);
    }

    /**
     * @brief Static method to print the comparison result between two durations with meaningful messages.
     * 
     * @param duration1 First duration in nanoseconds.
     * @param duration2 Second duration in nanoseconds.
     */
    static void printComparison(long long duration1, std::string duration1_tag, long long duration2, std::string duration2_tag)
    {
        std::tuple<long long, double, std::string> comparisonResult = compareDurations(duration1, duration2);
        long long difference = std::get<0>(comparisonResult);
        double ratio = std::get<1>(comparisonResult);
        std::string comparison = std::get<2>(comparisonResult);

        std::cout << "\nDuration Comparison:" << std::endl;
        std::cout << "Difference: " << difference << " nanoseconds." << std::endl;

        if (duration2 != 0) {
            std::cout << "Ratio: " << std::fixed << std::setprecision(2) << ratio << " times." << std::endl;
        }
        else {
            std::cout << "Ratio: Undefined (Second duration is zero)." << std::endl;
        }

        std::cout << comparison << std::endl;

        // Providing meaningful messages based on the ratio
        if (duration2 != 0) {
            if (ratio > 1.0) {
                std::cout << duration1_tag << " " << ratio << " times slower than the "<< duration2_tag << std::endl;
            }
            else if (ratio < 1.0) {
                std::cout << duration1_tag << " " << (1.0 / ratio) << " times faster than the "<< duration2_tag << std::endl;
            }
            else {
                // ratio == 1.0, already handled in comparison string
            }
        }
    }

    // Delete copy constructor and copy assignment to prevent copying
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    // Allow move constructor and move assignment
    Timer(Timer&&) = default;
    Timer& operator=(Timer&&) = default;

private:
    std::string message_;
    std::chrono::high_resolution_clock::time_point start_time_point_;
    long long duration_nanoseconds_;
    bool stopped_;
};


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

    SIMD::Array<SIMD::int_256<int16_t>, 15000> testArray;
    std::array<int16_t, testArray.Length * SIMD::int_256<int16_t>::ElementCount> plainArray;
    for(int i=0; i< testArray.Length; i++)
    {
        for(int j=0; j< testArray[i].ElementCount; j++)
        {
            testArray[i][j] = j;
            plainArray[i*SIMD::int_256<int16_t>::ElementCount + j] = j;
        }
    }

    SIMD::Array<SIMD::int_256<int16_t>, 15000> testArray2;
    std::array<int16_t, testArray2.Length * SIMD::int_256<int16_t>::ElementCount> plainArray2;
    for(int i=0; i< testArray.Length; i++)
    {
        for(int j=0; j< testArray2[i].ElementCount; j++)
        {
            testArray2[i][j] = j;
            plainArray2[i*SIMD::int_256<int16_t>::ElementCount + j] = j;
        }
    }

    long long durationSIMD = 0;
    long long durationPlain = 0;
    {
        Timer timer("SIMD Array Addition\t");
        testArray += testArray2;
        timer.stop();
        durationSIMD = timer.getDuration();
    }


    {
        Timer timer("Plain Array Addition\t");
        for(int i=0; i< plainArray.size(); i++)
        {
            plainArray[i] += plainArray2[i];
        }
        timer.stop();
        durationPlain = timer.getDuration();
    }

    Timer::printComparison(durationSIMD, "SIMD Sum", durationPlain, "Regular Sum");

    // //Print results for both arrays
    // for(int i=0; i< testArray.Length; i++)
    // {
    //     for(int j=0; j< testArray[i].ElementCount; j++)
    //     {
    //         std::cout<<"testArray.SIMDElements["<<i<<"]["<<j<<"]:"<<testArray[i][j]<<std::endl;
    //     }
    // }

    //     for(int i=0; i< testArray2.Length; i++)
    // {
    //     for(int j=0; j< testArray2[i].ElementCount; j++)
    //     {
    //         std::cout<<"testArray2.SIMDElements["<<i<<"]["<<j<<"]:"<<testArray2[i][j]<<std::endl;
    //     }
    // }
}

