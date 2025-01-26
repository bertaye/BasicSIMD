#pragma once

//SIMD Helper functions for generic SIMD operations

#include <type_traits>
#include <immintrin.h>
#include <iostream>

#define INTERNAL_SIMD_TYPE_NAME(DATA_TYPE, SIZE) SIMD_##DATA_TYPE##_##SIZE


#define GENERATE_SIMD(TYPE, XXX) \
template<>\
struct SIMD_Type_t<TYPE, XXX> : std::true_type \
{\
    SIMD_Type_t() : Data(nullptr) {}\
    /* Move constructor */ \
    SIMD_Type_t(SIMD_Type_t&& other) noexcept : Data(other.Data) { \
        other.Data = nullptr; \
        /*Load SIMD*/\
    } \
    /* Copy assignment operator */ \
    SIMD_Type_t& operator=(const SIMD_Type_t& other) { \
        if (this != &other) { \
            Data = other.Data; \
        } \
        return *this; \
    } \
    /* Move assignment operator */ \
    SIMD_Type_t& operator=(SIMD_Type_t&& other) noexcept { \
        if (this != &other) { \
            /*delete[] Data; How to remove Data??? */ \
            Data = other.Data; \
            other.Data = nullptr; \
        } \
        return *this; \
    } \
    explicit operator bool() noexcept { \
        return Data == nullptr; \
    } \
    using Type = TYPE;\
    static constexpr unsigned int Size = XXX;\
    void* Data;\
};\
namespace SIMD { using TYPE##_##XXX = SIMD_Type_t<TYPE, XXX>; }

template<typename T, int Size = 0>
struct SIMD_Type_t : std::false_type {};


GENERATE_SIMD(float, 512);
GENERATE_SIMD(int, 512);
GENERATE_SIMD(double, 512);
GENERATE_SIMD(float, 256);
GENERATE_SIMD(int, 256);
GENERATE_SIMD(double, 256);
GENERATE_SIMD(float, 128);
GENERATE_SIMD(int, 128);
GENERATE_SIMD(double, 128);

template<typename T>
using IsSIMDType = typename std::enable_if<SIMD_Type_t<typename T::Type, T::Size>::value, int>::type;

template<typename T>
using IsSIMD_Int = typename std::enable_if<std::is_same<typename T::Type, int>::value, int>::type;

template<typename T>
using IsSIMD_Float = typename std::enable_if<std::is_same<typename T::Type, float>::value, int>::type;

template<typename T>
using IsSIMD_Double = typename std::enable_if<std::is_same<typename T::Type, double>::value, int>::type;

template <typename T>
using IsElementAnyOfInts = typename std::enable_if<
                                (std::is_same<T, uint8_t>::value || std::is_same<T, int8_t>::value  ||
                                std::is_same<T, uint16_t>::value || std::is_same<T, int16_t>::value ||
                                std::is_same<T, uint32_t>::value || std::is_same<T, int32_t>::value || 
                                std::is_same<T, uint64_t>::value || std::is_same<T, int64_t>::value), int>::type;
class SIMDManager
{
public:
    #ifdef _MSC_VER
    #include <intrin.h>
    #define GET_CPU_INFO(info, x) __cpuidex(reinterpret_cast<int *>(info), x, 0)
    #else
    #include <cpuid.h>
    #define GET_CPU_INFO(info, x) __cpuid_count(x, 0, info[0], info[1], info[2], info[3])
    #endif
    //Singleton.
    SIMDManager()
    {
        unsigned int cpuInfo[4];
    	//Get SIMD capabilities
        /*
        SSE -> __m128,
        SSE2 -> __m128i, __m128d,
        AVX -> __m256, __m256i, __m256d,
        AVX512 -> __m512, __m512i, __m512d
        */
        GET_CPU_INFO(cpuInfo, 1);
        __m128_available    = cpuInfo[SSE_CPU_IDX] & SSE_FLAG;
        std::cout<<"__m128_available: "<<__m128_available<<std::endl;
        __m128i_available   = cpuInfo[SSE2_CPU_IDX] & SSE2_FLAG;
        std::cout<<"__m128i_available: "<< __m128i_available <<std::endl;

        __m128d_available   = cpuInfo[SSE2_CPU_IDX] & SSE2_FLAG;
        std::cout<<"__m128d_available: "<< __m128d_available <<std::endl;

        __m256_available    = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        std::cout<<"__m256_available: "<<__m128_available<<std::endl;
		__m256i_available   = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        std::cout<<"__m256i_available: "<<__m256i_available<<std::endl;
		__m256d_available   = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        std::cout<<"__m256d_available: "<<__m256d_available<<std::endl;

		__m512_available    = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        std::cout<<"__m512_available: "<<__m512_available<<std::endl;
		__m512i_available   = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        std::cout<<"__m512i_available: "<<__m512i_available<<std::endl;
		__m512d_available   = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        std::cout<<"__m512d_available: "<<__m512d_available<<std::endl;
    }
    SIMDManager(SIMDManager const&) = delete;
    void operator=(SIMDManager const&) = delete;
    static SIMDManager& GetInstance() {
	    static SIMDManager instance;
	    return instance;
    }

    template<typename T, typename T_Element, typename...Args, IsSIMDType<T> = 0, IsSIMD_Int<T> = 0, IsElementAnyOfInts<T_Element> = 0 >
    T CreateSIMDVariable(Args...args)
    {
        static_assert(sizeof...(Args) != 0 && sizeof(args) != T::Size, "Invalid number of arguments for SIMD Type");
        static_assert(sizeof...(Args) != T::Size/sizeof(T_Element), "Requested SIMD type can't hold all arguments of T_Element type")
        if (T::Size <= getTypeMaxAvailable<T>())
        {
            if (T::Size == 512)
            {
               alignas(64) T_Element data[T::Size / sizeof(T_Element)] = { args... };
               T simdVar;
               simdVar.Data = reinterpret_cast<void*>(_mm512_load_si512(reinterpret_cast<const __m512i*>(data)));
               std::cout<<"SIMD Variable created successfully"<<std::endl;
            }
			else if (T::Size == 256)
			{
				alignas(32) T_Element data[T::Size / sizeof(T_Element)] = { args... };
				T simdVar;
				simdVar.Data = reinterpret_cast<void*>(_mm256_loadu_epi32(reinterpret_cast<const __m256i*>(data)));
				std::cout<<"SIMD Variable created successfully"<<std::endl;
			}
        }
        else
        {
            throw std::runtime_error("SIMD Type of size " + std::to_string(T::Size) + " not available.");
        }
    }

    template<typename T, IsSIMDType<T> = 0>
    void PrintSIMDVarable(const T& simdVar)
	{
		if (T::Size == 256)
		{
			__m256i* simdData = reinterpret_cast<const __m256i*>(simdVar.Data);
            // Store back into an aligned array
            alignas(32) int32_t result[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(result), *simdData);

            // Print the results
            std::cout << "Processed SIMD data: ";
            for (int i = 0; i < 8; i++) {
                std::cout << result[i] << " ";
            }
            std::cout << std::endl;
		}
	}

    template<typename T, IsSIMDType<T> = 0, IsSIMD_Float<T> = 0 >
    int getTypeMaxAvailable() const
    {
        return  (512 * ((int)__m512_available)) +
                (256 * ((int)(!__m512_available && __m256_available))) +
                (128 * ((int)(!__m512_available && !__m256_available && __m128_available)));
    }

    template<typename T, IsSIMDType<T> = 0, IsSIMD_Int<T> = 0 >
	int getTypeMaxAvailable() const
	{
        return  (512 * ((int)__m512i_available)) +
            (256 * ((int)(!__m512i_available && __m256i_available))) +
            (128 * ((int)(!__m512i_available && !__m256i_available && __m128i_available)));
	}

    template<typename T, IsSIMDType<T> = 0, IsSIMD_Double<T> = 0 >
    int getTypeMaxAvailable() const
    {
        return  (512 * ((int)__m512d_available)) +
                (256 * ((int)(!__m512d_available && __m256d_available))) +
                (128 * ((int)(!__m512d_available && !__m256d_available && __m128d_available)));
    }

private:
    const int SSE_FLAG      = 1 << 25,       SSE_CPU_IDX = 3;
    const int SSE2_FLAG     = 1 << 26,      SSE2_CPU_IDX =3;
    const int AVX_FLAG      = 1 << 28,       AVX_CPU_IDX = 3;
    const int AVX2_FLAG     = 1 << 5,       AVX2_CPU_IDX = 1;
    const int AVX512_FLAG   = 1 << 16,    AVX512_CPU_IDX = 1;

    bool __m512_available = false;
    bool __m512i_available = false;
    bool __m512d_available = false;
    
    bool __m256_available = false;
    bool __m256i_available = false;
    bool __m256d_available = false;

    bool __m128_available = false;
    bool __m128i_available = false;
    bool __m128d_available = false;
};


#undef GENERATE_SIMD
#undef INTERNAL_SIMD_TYPE_NAME
