#pragma once
#include <type_traits>
#include <immintrin.h>
#include <iostream>
#include <stdint.h>
#include <array>
#ifdef _WIN32
#include <malloc.h>
#elif defined(__linux__)
#include <stdlib.h>
#endif

/*
 posix_memalign() returns zero on success, or one of the error
       values listed in the next section on failure.  The value of errno
       is not set.  On Linux (and other systems), posix_memalign() does
       not modify memptr on failure.  A requirement standardizing this
       behavior was added in POSIX.1-2008 TC2.
*/
static int allocate_aligned(void*& ptr, size_t size, size_t alignment)
{
#ifdef _WIN32
    errno_t err;
    ptr = NULL;
    ptr = _aligned_malloc(size, alignment);
    if (ptr == NULL)
    {
        _get_errno(&err);
        return (int)err;
    }
    return 0;
#elif defined(__linux__)
    int ret = posix_memalign(&ptr, alignment, size);
    if (ret)
    {
        ptr = nullptr;
    }
    return ret;
#endif
    return -1;
}

static void free_aligned(void*& ptr)
{
#ifdef _WIN32
    _aligned_free(ptr);
#elif defined(__linux__)
    free(ptr);
#endif
}


template<typename T, int Size = 0, typename T_ElementType = int32_t>
struct SIMD_Type_t : std::false_type {};

#define GENERATE_SIMD_INT(XXX) \
template<typename T_ElementType>\
struct SIMD_Type_t<int, XXX, T_ElementType> : std::true_type\
{\
    SIMD_Type_t() :Data(nullptr)\
    {\
		if (XXX > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<int, XXX, T_ElementType>>())\
		{\
			return;\
		}\
        int success = allocate_aligned(Data, SizeBytes, XXX / 8);\
        std::cout<<"Allocation status: "<<success<<std::endl;\
    }\
    template<typename ...Args,\
                    IsElementAnyOfInts<T_ElementType> = 0, \
                    IsAllElementsCompatible<T_ElementType, Args...> = 0, \
                    IsSizeValid< (XXX/8) / sizeof(T_ElementType), Args...> = 0>\
    SIMD_Type_t(Args... args) : Data(nullptr) \
    {\
        \
        if (XXX > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<int, XXX, T_ElementType>>())\
        {\
            return;\
        }\
        alignas(XXX / 8) T_ElementType data[SizeBytes / sizeof(T_ElementType)] = { args... };\
        int success = allocate_aligned(Data, SizeBytes, XXX / 8);\
        if (success == 0)\
        {\
            std::copy(data, data + SizeBytes / sizeof(T_ElementType), reinterpret_cast<T_ElementType*>(Data));\
        }\
        std::cout << "Allocation status: " << success << std::endl;\
    }\
    /* Move constructor */ \
    SIMD_Type_t(SIMD_Type_t&& other) noexcept : Data(other.Data) { \
        other.Data = nullptr; \
        /*Load SIMD*/\
    } \
    const T_ElementType *const Get()\
    {\
        return reinterpret_cast<T_ElementType*>(Data);\
    }\
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
            free_aligned(Data); \
            Data = other.Data; \
            other.Data = nullptr; \
        } \
        return *this; \
    } \
    explicit operator bool() noexcept { \
        return Data != nullptr; \
    }\
    \
    SIMD_Type_t operator+(const SIMD_Type_t& other) const\
    {\
        return SIMD_Type_t<int, XXX, T_ElementType>();\
    }\
    \
    using Type = int;\
    static constexpr unsigned int Size = XXX;\
    static constexpr unsigned int SizeBytes = XXX/8;\
    void* Data;\
};\
namespace SIMD \
{\
    template<typename ElementType=int32_t>\
    using int_##XXX = SIMD_Type_t<int, XXX,ElementType>; \
}\



#define CREATE_INT128_OPERATOR_PLUS(XX) \
template<>\
SIMD_Type_t<int, 128, int##XX##_t> SIMD_Type_t<int, 128, int##XX##_t>::operator+(const SIMD_Type_t<int, 128, int##XX##_t>& other) const\
{\
	SIMD_Type_t<int, 128, int##XX##_t> result;\
    if(128 > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<int, 128, int##XX##_t>>())\
    {\
        return result;\
    }\
	__m128i simdData1 = _mm_load_si128(reinterpret_cast<__m128i*>(Data));\
	__m128i simdData2 = _mm_load_si128(reinterpret_cast<__m128i*>(other.Data));\
    __m128i simdResult= _mm_add_epi##XX(simdData1, simdData2);\
	_mm_store_si128(reinterpret_cast<__m128i*>(result.Data), simdResult);\
	return result;\
}

#define CREATE_INT256_OPERATOR_PLUS(XX) \
template<>\
SIMD_Type_t<int, 256, int##XX##_t> SIMD_Type_t<int, 256, int##XX##_t>::operator+(const SIMD_Type_t<int, 256, int##XX##_t>& other) const\
{\
	SIMD_Type_t<int, 256, int##XX##_t> result;\
    if(256 > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<int, 256, int##XX##_t>>())\
    {\
        return result;\
    }\
	__m256i simdData1 = _mm256_load_si256(reinterpret_cast<__m256i*>(Data));\
	__m256i simdData2 = _mm256_load_si256(reinterpret_cast<__m256i*>(other.Data));\
    __m256i simdResult= _mm256_add_epi##XX(simdData1, simdData2);\
	_mm256_store_si256(reinterpret_cast<__m256i*>(result.Data), simdResult);\
	return result;\
}

#define CREATE_INT512_OPERATOR_PLUS(XX) \
template<>\
SIMD_Type_t<int, 512, int##XX##_t> SIMD_Type_t<int, 512, int##XX##_t>::operator+(const SIMD_Type_t<int, 512, int##XX##_t>& other) const\
{\
	SIMD_Type_t<int, 512, int##XX##_t> result;\
    if(512 > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<int, 512, int##XX##_t>>())\
    {\
        return result;\
    }\
    __m512i simdData1 = _mm512_load_si512(reinterpret_cast<__m512i*>(Data));\
	__m512i simdData2 = _mm512_load_si512(reinterpret_cast<__m512i*>(other.Data));\
    __m512i simdResult= _mm512_add_epi##XX(simdData1, simdData2);\
	_mm512_store_si512(reinterpret_cast<__m512i*>(result.Data), simdResult);\
	return result;\
}

template<typename T>
using IsSIMD_Int = typename std::enable_if<std::is_same<typename T::Type, int>::value, int>::type;

template<typename T>
using IsSIMD_Float = typename std::enable_if<std::is_same<typename T::Type, float>::value, int>::type;

template<typename T>
using IsSIMD_Double = typename std::enable_if<std::is_same<typename T::Type, double>::value, int>::type;

template<typename T>
using IsElementAnyOfInts = typename std::enable_if< std::is_same<T, uint8_t>::value || std::is_same<T, int8_t>::value ||
    std::is_same<T, uint16_t>::value || std::is_same<T, int16_t>::value ||
    std::is_same<T, uint32_t>::value || std::is_same<T, int32_t>::value ||
    std::is_same<T, uint64_t>::value || std::is_same<T, int64_t>::value, int>::type;


// Base case: If no Args are provided, return true
template<typename T_ElementType, typename... Args>
using IsArrayConstructible = typename std::enable_if<
    std::is_constructible<std::array<T_ElementType, sizeof...(Args)>, Args...>::value,
    int
>::type;

template<int Size, typename... Args>
using IsSizeValid = typename std::enable_if<(Size >= static_cast<int>(sizeof...(Args))), int>::type;


template<typename TypeRequired, typename FirstElementType, typename... Rest>
struct IsAllElementsCompatible_impl : std::conditional<
    std::is_convertible<TypeRequired, FirstElementType>::value,
    IsAllElementsCompatible_impl<TypeRequired, Rest...>,
    std::false_type>::type {};

template<typename TypeRequired, typename FirstElementType>
struct IsAllElementsCompatible_impl<TypeRequired, FirstElementType> : std::conditional<
    std::is_convertible<TypeRequired, FirstElementType>::value,
    std::true_type,
    std::false_type>::type {};

template<typename T, typename... Elements>
using IsAllElementsCompatible = typename std::enable_if<IsAllElementsCompatible_impl<T, Elements...>::value, int>::type;

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
        __m128_available = cpuInfo[SSE_CPU_IDX] & SSE_FLAG;
        std::cout << "__m128_available: " << __m128_available << std::endl;
        __m128i_available = cpuInfo[SSE2_CPU_IDX] & SSE2_FLAG;
        std::cout << "__m128i_available: " << __m128i_available << std::endl;

        __m128d_available = cpuInfo[SSE2_CPU_IDX] & SSE2_FLAG;
        std::cout << "__m128d_available: " << __m128d_available << std::endl;

        __m256_available = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        std::cout << "__m256_available: " << __m128_available << std::endl;
        __m256i_available = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        std::cout << "__m256i_available: " << __m256i_available << std::endl;
        __m256d_available = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        std::cout << "__m256d_available: " << __m256d_available << std::endl;

        __m512_available = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        std::cout << "__m512_available: " << __m512_available << std::endl;
        __m512i_available = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        std::cout << "__m512i_available: " << __m512i_available << std::endl;
        __m512d_available = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        std::cout << "__m512d_available: " << __m512d_available << std::endl;
    }
    SIMDManager(SIMDManager const&) = delete;
    void operator=(SIMDManager const&) = delete;
    static SIMDManager& GetInstance() {
        static SIMDManager instance;
        return instance;
    }

    template<typename T/*, IsSIMDType<T> = 0*/>
    void PrintSIMDVariable(const T& simdVar)
    {
        if (T::Size == 256)
        {
            __m256i* simdData = reinterpret_cast<__m256i*>(simdVar.Data);
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

    template<typename T, /*IsSIMDType<T> = 0,*/ IsSIMD_Float<T> = 0 >
    int getTypeMaxAvailable() const
    {
        return  (512 * ((int)__m512_available)) +
            (256 * ((int)(!__m512_available && __m256_available))) +
            (128 * ((int)(!__m512_available && !__m256_available && __m128_available)));
    }

    template<typename T,/* IsSIMDType<T> = 0,*/ IsSIMD_Int<T> = 0 >
    int getTypeMaxAvailable() const
    {
        return  (512 * ((int)__m512i_available)) +
            (256 * ((int)(!__m512i_available && __m256i_available))) +
            (128 * ((int)(!__m512i_available && !__m256i_available && __m128i_available)));
    }

    template<typename T,/* IsSIMDType<T> = 0,*/ IsSIMD_Double<T> = 0 >
    int getTypeMaxAvailable() const
    {
        return  (512 * ((int)__m512d_available)) +
            (256 * ((int)(!__m512d_available && __m256d_available))) +
            (128 * ((int)(!__m512d_available && !__m256d_available && __m128d_available)));
    }

private:
    const int SSE_FLAG = 1 << 25, SSE_CPU_IDX = 3;
    const int SSE2_FLAG = 1 << 26, SSE2_CPU_IDX = 3;
    const int AVX_FLAG = 1 << 28, AVX_CPU_IDX = 3;
    const int AVX2_FLAG = 1 << 5, AVX2_CPU_IDX = 1;
    const int AVX512_FLAG = 1 << 16, AVX512_CPU_IDX = 1;

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

GENERATE_SIMD_INT(128);
CREATE_INT128_OPERATOR_PLUS(16);
CREATE_INT128_OPERATOR_PLUS(32);
CREATE_INT128_OPERATOR_PLUS(64);

GENERATE_SIMD_INT(256);
CREATE_INT256_OPERATOR_PLUS(16);
CREATE_INT256_OPERATOR_PLUS(32);
CREATE_INT256_OPERATOR_PLUS(64);

GENERATE_SIMD_INT(512);
CREATE_INT512_OPERATOR_PLUS(16);
CREATE_INT512_OPERATOR_PLUS(32);
CREATE_INT512_OPERATOR_PLUS(64);

#undef GENERATE_SIMD
#undef INTERNAL_SIMD_TYPE_NAME
