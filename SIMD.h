#ifdef _MSC_VER
#include <intrin.h>
#define GET_CPU_INFO(info, x) __cpuidex(reinterpret_cast<int *>(info), x, 0)
#define _SIMD_INL_ __forceinline
#else
#include <cpuid.h>
#define GET_CPU_INFO(info, x) __cpuid_count(x, 0, info[0], info[1], info[2], info[3])
#define _SIMD_INL_ __attribute__((always_inline))
#endif
#include <immintrin.h>
#include <type_traits>
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


//       _        ___       _        _________________
//      | |      |   \     | |      |_______   _______|
//      | |      | |\ \    | |              | |
//      | |      | | \ \   | |              | |
//      | |      | |  \ \  | |              | |
//      | |      | |   \ \ | |              | |
//      | |      | |    \ \| |              | |
//      | |      | |     \   |              | |
//      |_|      |_|      \ _|              |_|


#define GENERATE_SIMD_INTERNAL_TYPE(TYPE, XXX) \
template<typename T_ElementType>\
struct SIMD_Type_t<TYPE, XXX, T_ElementType> : std::true_type\
{\
    SIMD_Type_t() :Data(nullptr), IsImported(false)\
    {\
        std::cout<<"Default constructor called"<<std::endl;\
		if (XXX > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<TYPE, XXX, T_ElementType>>())\
		{\
			return;\
		}\
        int success = allocate_aligned(Data, SizeBytes, XXX / 8);\
    }\
    template<typename ...Args,\
                    IsElementValid<TYPE, T_ElementType> = 0, \
                    IsAllElementsCompatible<T_ElementType, Args...> = 0, \
                    IsSizeValid< (XXX/8) / sizeof(T_ElementType), Args...> = 0>\
    SIMD_Type_t(Args... args) : Data(nullptr), IsImported(false) \
    {\
        std::cout::<<"Default constructor with args called"<<std::endl;\
        if (XXX > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<TYPE, XXX, T_ElementType>>())\
        {\
            return;\
        }\
        alignas(XXX / 8) T_ElementType data[SizeBytes / sizeof(T_ElementType)] = { args... };\
        int success = allocate_aligned(Data, SizeBytes, XXX / 8);\
        if (success == 0)\
        {\
            std::copy(data, data + SizeBytes / sizeof(T_ElementType), reinterpret_cast<T_ElementType*>(Data));\
        }\
    }\
    SIMD_Type_t(void* data) : Data(nullptr), IsImported(false)\
    {\
        /*This will check for memory alignment*/\
        if(XXX > SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<TYPE, XXX, T_ElementType>>() || (reinterpret_cast<uintptr_t>(data) & (Alignment - 1)) != 0)\
        {\
        std::cout<<"MaxAvailable Type:"<<SIMDManager::GetInstance().getTypeMaxAvailable<SIMD_Type_t<TYPE, XXX, T_ElementType>>()<<std::endl;\
        std::cout<<"Is Alignment Valid: "<<((reinterpret_cast<uintptr_t>(data) & (Alignment - 1)) == 0)<<std::endl;\
         return;\
        }\
        IsImported = true;\
        Data = data;\
    }\
    /* Move constructor */ \
    SIMD_Type_t(SIMD_Type_t&& other) noexcept : Data(other.Data), IsImported(other.IsImported) { \
        std::cout<<"Move constructor called"<<std::endl;\
        other.Data = nullptr; \
        other.IsImported = false;\
        /*Load SIMD*/\
    } \
    SIMD_Type_t(const SIMD_Type_t& other) noexcept : Data(nullptr), IsImported(false) { \
        std::cout<<"Copy constructor called"<<std::endl;\
        int success = allocate_aligned(Data, SizeBytes, XXX / 8);\
        if (success == 0)\
        {\
            std::copy(other.Data, other.Data + SizeBytes / sizeof(T_ElementType), reinterpret_cast<T_ElementType*>(Data));\
        }\
    } \
    ~SIMD_Type_t()\
    {\
        if(!IsImported)\
        {\
            free_aligned(Data);\
        }\
    }\
    static _SIMD_INL_ SIMD_Type_t Import(void* data)\
    {\
        return std::move(SIMD_Type_t(data));\
    }\
    const T_ElementType *const Get()\
    {\
        return reinterpret_cast<T_ElementType*>(Data);\
    }\
    /* Copy assignment operator */ \
    SIMD_Type_t& operator=(const SIMD_Type_t& other) { \
        std::cout<<"Using default copy assignment operator"<<std::endl;\
        if (this != &other && Data && other.Data) { \
            _mm_store_si128((__m128i*)Data, _mm_load_si128((__m128i*)other.Data)); \
        }\
        return *this; \
    } \
    explicit operator bool() noexcept { \
        return Data != nullptr; \
    }\
    \
    T_ElementType ElementAt(unsigned int index) const\
	{\
        if(index >= ElementCount)\
		{\
			return 0;\
		}\
		return *(reinterpret_cast<T_ElementType*>(Data) + index);\
	}\
    static _SIMD_INL_ SIMD_Type_t Add(const SIMD_Type_t& lhs, const SIMD_Type_t& rhs) {\
        SIMD_Type_t result;\
        /* Implementation will be specialized */\
        return result;\
    }\
    static _SIMD_INL_ void AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from) {\
        /* Implementation will be specialized */\
    }\
    static _SIMD_INL_ void AddInplaceRaw(T_ElementType* to, const T_ElementType* from) {\
        /* Implementation will be specialized */\
    }\
    \
    static _SIMD_INL_ SIMD_Type_t Subtract(const SIMD_Type_t& lhs, const SIMD_Type_t& rhs) {\
        SIMD_Type_t result;\
        /* Implementation will be specialized */\
        return result;\
    }\
    static _SIMD_INL_ void SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from) {\
        /* Implementation will be specialized */\
    }\
    static _SIMD_INL_ void SubtractInplaceRaw(T_ElementType* to, const T_ElementType* from) {\
        /* Implementation will be specialized */\
    }\
    \
    static _SIMD_INL_ SIMD_Type_t Multiply(const SIMD_Type_t& lhs, const SIMD_Type_t& rhs) {\
        SIMD_Type_t result;\
        /* Implementation will be specialized */\
        return result;\
    }\
    static _SIMD_INL_ void MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from) {\
        /* Implementation will be specialized */\
    }\
    static _SIMD_INL_ void MultiplyInplaceRaw(T_ElementType* to, const T_ElementType* from) {\
        /* Implementation will be specialized */\
    }\
    \
    _SIMD_INL_ SIMD_Type_t operator+(const SIMD_Type_t& other) const\
    {\
        return Add(*this, other);\
    }\
    _SIMD_INL_ SIMD_Type_t operator*(const SIMD_Type_t& other) const\
    {\
        return Multiply(*this, other);\
    }\
    _SIMD_INL_ SIMD_Type_t operator-(const SIMD_Type_t& other) const\
    {\
        return Subtract(*this, other);\
    }\
    _SIMD_INL_ void operator+=(const SIMD_Type_t& other)\
    {\
        AddInplace(*this, other);\
    }\
    _SIMD_INL_ void operator*=(const SIMD_Type_t& other)\
    {\
        MultiplyInplace(*this, other);\
    }\
    void operator-=(const SIMD_Type_t& other)\
    {\
        SubtractInplace(*this, other);\
    }\
    _SIMD_INL_ T_ElementType& operator[](unsigned int index)\
    {\
        return *(reinterpret_cast<T_ElementType*>(Data) + index);\
    }\
    \
    using Type = typename TYPE;\
    using ElementType = typename T_ElementType;\
    static constexpr unsigned int BitWidth = XXX;\
    static constexpr unsigned int SizeBytes = XXX/8;\
    static constexpr unsigned int Alignment = XXX/8;\
    static constexpr unsigned int ElementCount = (XXX/8)/sizeof(T_ElementType);\
    void* Data;\
private:\
    bool IsImported;\
};

#define DECLARE_SIMD_USE_TYPE_INT(TYPE, XXX) \
namespace SIMD \
 {\
    template<typename ElementType=int32_t>\
    using TYPE##_##XXX = SIMD_Type_t<TYPE, XXX, ElementType>; \
 }

#define DECLARE_SIMD_USE_TYPE(TYPE, XXX) \
namespace SIMD \
 {\
    using TYPE##_##XXX = SIMD_Type_t<TYPE, XXX, TYPE>; \
 }

 #define CREATE_INT128_OPERATOR_ASSIGN(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, int##XX##_t>& SIMD_Type_t<int, 128, int##XX##_t>::operator=(const SIMD_Type_t<int, 128, int##XX##_t>& other) \
{\
    Data = other.Data;\
    return *this; \
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, uint##XX##_t>& SIMD_Type_t<int, 128, uint##XX##_t>::operator=(const SIMD_Type_t<int, 128, uint##XX##_t>& other) \
{\
    Data = other.Data;\
    return *this; \
}


#define CREATE_INT128_OPERATOR_PLUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, int##XX##_t> SIMD_Type_t<int, 128, int##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm_store_si128((__m128i*)result.Data, _mm_add_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, uint##XX##_t> SIMD_Type_t<int, 128,uint##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm_store_si128((__m128i*)result.Data, _mm_add_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, int##XX##_t>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm_store_si128((__m128i*)to.Data, _mm_add_epi##XX(_mm_load_si128((__m128i*)to.Data), _mm_load_si128((__m128i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, uint##XX##_t>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm_store_si128((__m128i*)to.Data, _mm_add_epi##XX(_mm_load_si128((__m128i*)to.Data), _mm_load_si128((__m128i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, int##XX##_t>::AddInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm_store_si128((__m128i*)to, _mm_add_epi##XX(_mm_load_si128((__m128i*)to), _mm_load_si128((__m128i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, uint##XX##_t>::AddInplaceRaw(uint##XX##_t* to, const  uint##XX##_t* from)\
{\
    _mm_store_si128((__m128i*)to, _mm_add_epi##XX(_mm_load_si128((__m128i*)to), _mm_load_si128((__m128i*)from)));\
}

#define CREATE_INT128_OPERATOR_MINUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, int##XX##_t> SIMD_Type_t<int, 128, int##XX##_t>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm_store_si128((__m128i*)result.Data, _mm_sub_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, uint##XX##_t> SIMD_Type_t<int, 128,uint##XX##_t>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm_store_si128((__m128i*)result.Data, _mm_sub_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, int##XX##_t>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm_store_si128((__m128i*)to.Data, _mm_sub_epi##XX(_mm_load_si128((__m128i*)to.Data), _mm_load_si128((__m128i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, uint##XX##_t>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm_store_si128((__m128i*)to.Data, _mm_sub_epi##XX(_mm_load_si128((__m128i*)to.Data), _mm_load_si128((__m128i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, int##XX##_t>::SubtractInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm_store_si128((__m128i*)to, _mm_sub_epi##XX(_mm_load_si128((__m128i*)to), _mm_load_si128((__m128i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, uint##XX##_t>::SubtractInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm_store_si128((__m128i*)to, _mm_sub_epi##XX(_mm_load_si128((__m128i*)to), _mm_load_si128((__m128i*)from)));\
}

#define CREATE_INT256_OPERATOR_ASSIGN(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, int##XX##_t>& SIMD_Type_t<int, 256, int##XX##_t>::operator=(const SIMD_Type_t<int, 256, int##XX##_t>& other) \
{\
    Data = other.Data;\
    return *this; \
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, uint##XX##_t>& SIMD_Type_t<int, 256, uint##XX##_t>::operator=(const SIMD_Type_t<int, 256, uint##XX##_t>& other) \
{\
    Data = other.Data;\
    return *this; \
}

#define CREATE_INT256_OPERATOR_PLUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, int##XX##_t> SIMD_Type_t<int, 256, int##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm256_store_si256((__m256i*)result.Data, _mm256_add_epi##XX(_mm256_load_si256((__m256i*)a.Data), _mm256_load_si256((__m256i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, uint##XX##_t> SIMD_Type_t<int, 256, uint##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm256_store_si256((__m256i*)result.Data, _mm256_add_epi##XX(_mm256_load_si256((__m256i*)a.Data), _mm256_load_si256((__m256i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, int##XX##_t>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm256_store_si256((__m256i*)to.Data, _mm256_add_epi##XX(_mm256_load_si256((__m256i*)to.Data), _mm256_load_si256((__m256i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, uint##XX##_t>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm256_store_si256((__m256i*)to.Data, _mm256_add_epi##XX(_mm256_load_si256((__m256i*)to.Data), _mm256_load_si256((__m256i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, int##XX##_t>::AddInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm256_store_si256((__m256i*)to, _mm256_add_epi##XX(_mm256_load_si256((__m256i*)to), _mm256_load_si256((__m256i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, uint##XX##_t>::AddInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm256_store_si256((__m256i*)to, _mm256_add_epi##XX(_mm256_load_si256((__m256i*)to), _mm256_load_si256((__m256i*)from)));\
}

#define CREATE_INT256_OPERATOR_MINUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, int##XX##_t> SIMD_Type_t<int, 256, int##XX##_t>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm256_store_si256((__m256i*)result.Data, _mm256_sub_epi##XX(_mm256_load_si256((__m256i*)a.Data), _mm256_load_si256((__m256i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, uint##XX##_t> SIMD_Type_t<int, 256, uint##XX##_t>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm256_store_si256((__m256i*)result.Data, _mm256_sub_epi##XX(_mm256_load_si256((__m256i*)a.Data), _mm256_load_si256((__m256i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, int##XX##_t>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm256_store_si256((__m256i*)to.Data, _mm256_sub_epi##XX(_mm256_load_si256((__m256i*)to.Data), _mm256_load_si256((__m256i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, uint##XX##_t>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm256_store_si256((__m256i*)to.Data, _mm256_sub_epi##XX(_mm256_load_si256((__m256i*)to.Data), _mm256_load_si256((__m256i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, int##XX##_t>::SubtractInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm256_store_si256((__m256i*)to, _mm256_sub_epi##XX(_mm256_load_si256((__m256i*)to), _mm256_load_si256((__m256i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, uint##XX##_t>::SubtractInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm256_store_si256((__m256i*)to, _mm256_sub_epi##XX(_mm256_load_si256((__m256i*)to), _mm256_load_si256((__m256i*)from)));\
}

#define CREATE_INT512_OPERATOR_ASSIGN(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, int##XX##_t>& SIMD_Type_t<int, 512, int##XX##_t>::operator=(const SIMD_Type_t<int, 512, int##XX##_t>& other) \
{\
    Data = other.Data;\
    return *this; \
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, uint##XX##_t>& SIMD_Type_t<int, 512, uint##XX##_t>::operator=(const SIMD_Type_t<int, 512, uint##XX##_t>& other) \
{\
    Data = other.Data;\
    return *this; \
}

#define CREATE_INT512_OPERATOR_PLUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, int##XX##_t> SIMD_Type_t<int, 512, int##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm512_store_si512((__m512i*)result.Data, _mm512_add_epi##XX(_mm512_load_si512((__m512i*)a.Data), _mm512_load_si512((__m512i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, uint##XX##_t> SIMD_Type_t<int, 512, uint##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm512_store_si512((__m512i*)result.Data, _mm512_add_epi##XX(_mm512_load_si512((__m512i*)a.Data), _mm512_load_si512((__m512i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, int##XX##_t>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm512_store_si512((__m512i*)to.Data, _mm512_add_epi##XX(_mm512_load_si512((__m512i*)to.Data), _mm512_load_si512((__m512i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, uint##XX##_t>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm512_store_si512((__m512i*)to.Data, _mm512_add_epi##XX(_mm512_load_si512((__m512i*)to.Data), _mm512_load_si512((__m512i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, int##XX##_t>::AddInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm512_store_si512((__m512i*)to, _mm512_add_epi##XX(_mm512_load_si512((__m512i*)to), _mm512_load_si512((__m512i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, uint##XX##_t>::AddInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm512_store_si512((__m512i*)to, _mm512_add_epi##XX(_mm512_load_si512((__m512i*)to), _mm512_load_si512((__m512i*)from)));\
}

#define CREATE_INT512_OPERATOR_MINUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, int##XX##_t> SIMD_Type_t<int, 512, int##XX##_t>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm512_store_si512((__m512i*)result.Data, _mm512_sub_epi##XX(_mm512_load_si512((__m512i*)a.Data), _mm512_load_si512((__m512i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, uint##XX##_t> SIMD_Type_t<int, 512, uint##XX##_t>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm512_store_si512((__m512i*)result.Data, _mm512_sub_epi##XX(_mm512_load_si512((__m512i*)a.Data), _mm512_load_si512((__m512i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, int##XX##_t>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm512_store_si512((__m512i*)to.Data, _mm512_sub_epi##XX(_mm512_load_si512((__m512i*)to.Data), _mm512_load_si512((__m512i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, uint##XX##_t>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm512_store_si512((__m512i*)to.Data, _mm512_sub_epi##XX(_mm512_load_si512((__m512i*)to.Data), _mm512_load_si512((__m512i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, int##XX##_t>::SubtractInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm512_store_si512((__m512i*)to, _mm512_sub_epi##XX(_mm512_load_si512((__m512i*)to), _mm512_load_si512((__m512i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, uint##XX##_t>::SubtractInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm512_store_si512((__m512i*)to, _mm512_sub_epi##XX(_mm512_load_si512((__m512i*)to), _mm512_load_si512((__m512i*)from)));\
}

//       _________      _              ___________      _____________     _________________
//      | ________|    | |            |  _______  |     | ________  |    |_______   _______|
//      | |            | |            | |       | |     | |       | |            | |
//      | |_______     | |            | |       | |     | |_______| |            | |
//      |  _______|    | |            | |       | |     |  _______  |            | |
//      | |            | |            | |       | |     | |       | |            | |
//      | |            | |            | |       | |     | |       | |            | |
//      | |            | |            | |       | |     | |       | |            | |
//      | |            | |________    | |_______| |     | |       | |            | |
//      |_|            |__________|   |___________|     |_|       |_|            |_|

#define CREATE_FLOAT_OPERATOR_ASSIGN(XXX) \
template<>\
_SIMD_INL_ SIMD_Type_t<float, XXX, float>& SIMD_Type_t<float, XXX, float>::operator=(const SIMD_Type_t<float, XXX, float>& other) \
{\
    Data = other.Data;\
    return *this; \
}

#define CREATE_FLOAT_OPERATOR_PLUS(XXX) \
template<>\
_SIMD_INL_ SIMD_Type_t<float, XXX, float> SIMD_Type_t<float, XXX, float>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm##XXX##_store_ps((float*)result.Data, _mm##XXX##_add_ps(_mm##XXX##_load_ps((float*)a.Data), _mm##XXX##_load_ps((float*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<float, XXX, float>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm##XXX##_store_ps((float*)to.Data, _mm##XXX##_add_ps(_mm##XXX##_load_ps((float*)to.Data), _mm##XXX##_load_ps((float*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<float, XXX, float>::AddInplaceRaw(float* to, const float* from)\
{\
    _mm##XXX##_store_ps((float*)to, _mm##XXX##_add_ps(_mm##XXX##_load_ps((float*)to), _mm##XXX##_load_ps((float*)from)));\
}

#define CREATE_FLOAT_OPERATOR_MULTIPLY(XXX) \
template<>\
_SIMD_INL_ SIMD_Type_t<float, XXX, float> SIMD_Type_t<float, XXX, float>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm##XXX##_store_ps((float*)result.Data, _mm##XXX##_mul_ps(_mm##XXX##_load_ps((float*)a.Data), _mm##XXX##_load_ps((float*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<float, XXX, float>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm##XXX##_store_ps((float*)to.Data, _mm##XXX##_mul_ps(_mm##XXX##_load_ps((float*)to.Data), _mm##XXX##_load_ps((float*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<float, XXX, float>::MultiplyInplaceRaw(float* to, const float* from)\
{\
    _mm##XXX##_store_ps((float*)to, _mm##XXX##_mul_ps(_mm##XXX##_load_ps((float*)to), _mm##XXX##_load_ps((float*)from)));\
}

#define CREATE_FLOAT_OPERATOR_MINUS(XXX) \
template<>\
_SIMD_INL_ SIMD_Type_t<float, XXX, float> SIMD_Type_t<float, XXX, float>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm##XXX##_store_ps((float*)result.Data, _mm##XXX##_sub_ps(_mm##XXX##_load_ps((float*)a.Data), _mm##XXX##_load_ps((float*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<float, XXX, float>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm##XXX##_store_ps((float*)to.Data, _mm##XXX##_sub_ps(_mm##XXX##_load_ps((float*)to.Data), _mm##XXX##_load_ps((float*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<float, XXX, float>::SubtractInplaceRaw(float* to, const float* from)\
{\
    _mm##XXX##_store_ps((float*)to, _mm##XXX##_sub_ps(_mm##XXX##_load_ps((float*)to), _mm##XXX##_load_ps((float*)from)));\
}


//  ________      ______     _        _    ______      _            _________ 
//  |  ___  \    / ____ \   | |      | |  | ____ \    | |          | ________|                                                                                                   
//  | |    \ \  | /    \ |  | |      | |  | |   \ \   | |          | |                                                                                                                                      
//  | |    | |  | |    | |  | |      | |  | |   / /   | |          | |_______                                                                    
//  | |    | |  | |    | |  | |      | |  | |  / /    | |          |  _______|                                                                   
//  | |    | |  | |    | |  | |      | |  | |  \ \    | |          | |                                                                           
//  | |    | |  | |    | |  | |      | |  | |   \ \   | |          | |                                                                           
//  | |____/ |  | \____/ |  | \______/ |  | |___/ /   | |________  | |_______                                                                           
//  |_______/    \______/    \________/   |______/    |__________| |_________|                                                                          

#define CREATE_DOUBLE_OPERATOR_ASSIGN(XXX) \
template<>\
_SIMD_INL_ SIMD_Type_t<double, XXX, double>& SIMD_Type_t<double, XXX, double>::operator=(const SIMD_Type_t<double, XXX, double>& other) \
{\
    Data = other.Data;\
    return *this; \
}

#define CREATE_DOUBLE_OPERATOR_PLUS(XXX)\
template<>\
_SIMD_INL_ SIMD_Type_t<double, XXX, double> SIMD_Type_t<double, XXX, double>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm##XXX##_store_pd((double*)result.Data, _mm##XXX##_add_pd(_mm##XXX##_load_pd((double*)a.Data), _mm##XXX##_load_pd((double*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<double, XXX, double>::AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm##XXX##_store_pd((double*)to.Data, _mm##XXX##_add_pd(_mm##XXX##_load_pd((double*)to.Data), _mm##XXX##_load_pd((double*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<double, XXX, double>::AddInplaceRaw(double* to, const double* from)\
{\
    _mm##XXX##_store_pd((double*)to, _mm##XXX##_add_pd(_mm##XXX##_load_pd((double*)to), _mm##XXX##_load_pd((double*)from)));\
}

#define CREATE_DOUBLE_OPERATOR_MINUS(XXX) \
template<>\
_SIMD_INL_ SIMD_Type_t<double, XXX, double> SIMD_Type_t<double, XXX, double>::Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm##XXX##_store_pd((double*)result.Data, _mm##XXX##_sub_pd(_mm##XXX##_load_pd((double*)a.Data), _mm##XXX##_load_pd((double*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<double, XXX, double>::SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm##XXX##_store_pd((double*)to.Data, _mm##XXX##_sub_pd(_mm##XXX##_load_pd((double*)to.Data), _mm##XXX##_load_pd((double*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<double, XXX, double>::SubtractInplaceRaw(double* to, const double* from)\
{\
    _mm##XXX##_store_pd((double*)to, _mm##XXX##_sub_pd(_mm##XXX##_load_pd((double*)to), _mm##XXX##_load_pd((double*)from)));\
}

template<typename T>
using IsSIMD_Int = typename std::enable_if<std::is_same<typename T::Type, int>::value, int>::type;

template<typename T>
using IsSIMD_Float = typename std::enable_if<std::is_same<typename T::Type, float>::value, int>::type;

template<typename T>
using IsSIMD_Double = typename std::enable_if<std::is_same<typename T::Type, double>::value, int>::type;

template<typename T>
struct IsElementAnyOfInts : std::integral_constant<
    bool,
    std::is_same<T, uint8_t>::value || std::is_same<T, int8_t>::value ||
    std::is_same<T, uint16_t>::value || std::is_same<T, int16_t>::value ||
    std::is_same<T, uint32_t>::value || std::is_same<T, int32_t>::value ||
    std::is_same<T, uint64_t>::value || std::is_same<T, int64_t>::value> {};

template<typename SIMD_Kind, typename ElementType>
struct IsElementValid_Impl : std::false_type {};

template<typename ElementType>
struct IsElementValid_Impl<int, ElementType> : IsElementAnyOfInts<ElementType> {};

template<>
struct IsElementValid_Impl<float, float> : std::true_type {};

template<>
struct IsElementValid_Impl<double, double> : std::true_type {};

template<typename SIMD_Kind, typename ElementType>
using IsElementValid = typename std::enable_if<IsElementValid_Impl<SIMD_Kind, ElementType>::value, int>::type;


template<typename Element, typename Target>
using IsElementAnyOfT = typename std::enable_if<std::is_same<Element, Target>::value, int>::type;


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
        __m128i_available = cpuInfo[SSE2_CPU_IDX] & SSE2_FLAG;

        __m128d_available = cpuInfo[SSE2_CPU_IDX] & SSE2_FLAG;

        __m256_available = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        __m256i_available = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;
        __m256d_available = cpuInfo[AVX_CPU_IDX] & AVX_FLAG;

        __m512_available = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        __m512i_available = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;
        __m512d_available = cpuInfo[AVX512_CPU_IDX] & AVX512_FLAG;

        // Query leaf 7 for AVX2 and AVX-512
        GET_CPU_INFO(cpuInfo, 7);
        // AVX2 and AVX-512 in EBX (index 1)
        __m256i_available = __m256_available && (cpuInfo[1] & (1 << 5));  // AVX2
        __m512_available = cpuInfo[1] & (1 << 16);  // AVX-512F
    }
    SIMDManager(SIMDManager const&) = delete;
    void operator=(SIMDManager const&) = delete;
    static SIMDManager& GetInstance() {
        static SIMDManager instance;
        return instance;
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
    const int AVX_FLAG = 1 << 28, AVX_CPU_IDX = 2;
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



#if defined(__SSE2__) || defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)

GENERATE_SIMD_INTERNAL_TYPE(int, 128);

CREATE_INT128_OPERATOR_ASSIGN(8);
CREATE_INT128_OPERATOR_ASSIGN(16);
CREATE_INT128_OPERATOR_ASSIGN(32);
CREATE_INT128_OPERATOR_ASSIGN(64);
CREATE_INT128_OPERATOR_PLUS(8);
CREATE_INT128_OPERATOR_PLUS(16);
CREATE_INT128_OPERATOR_PLUS(32);
CREATE_INT128_OPERATOR_PLUS(64);
CREATE_INT128_OPERATOR_MINUS(8);
CREATE_INT128_OPERATOR_MINUS(16);
CREATE_INT128_OPERATOR_MINUS(32);
CREATE_INT128_OPERATOR_MINUS(64);

DECLARE_SIMD_USE_TYPE_INT(int, 128);



GENERATE_SIMD_INTERNAL_TYPE(int, 256);

CREATE_INT256_OPERATOR_ASSIGN(8);
CREATE_INT256_OPERATOR_ASSIGN(16);
CREATE_INT256_OPERATOR_ASSIGN(32);
CREATE_INT256_OPERATOR_ASSIGN(64);
CREATE_INT256_OPERATOR_PLUS(8);
CREATE_INT256_OPERATOR_PLUS(16);
CREATE_INT256_OPERATOR_PLUS(32);
CREATE_INT256_OPERATOR_PLUS(64);
CREATE_INT256_OPERATOR_MINUS(8);
CREATE_INT256_OPERATOR_MINUS(16);
CREATE_INT256_OPERATOR_MINUS(32);
CREATE_INT256_OPERATOR_MINUS(64);

DECLARE_SIMD_USE_TYPE_INT(int, 256);

#endif

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)

GENERATE_SIMD_INTERNAL_TYPE(float, 256);

CREATE_FLOAT_OPERATOR_ASSIGN(256);
CREATE_FLOAT_OPERATOR_PLUS(256);
CREATE_FLOAT_OPERATOR_MINUS(256);
CREATE_FLOAT_OPERATOR_MULTIPLY(256);

DECLARE_SIMD_USE_TYPE(float, 256);


GENERATE_SIMD_INTERNAL_TYPE(double, 256);

CREATE_DOUBLE_OPERATOR_ASSIGN(256);
CREATE_DOUBLE_OPERATOR_PLUS(256);
CREATE_DOUBLE_OPERATOR_MINUS(256);

DECLARE_SIMD_USE_TYPE(double, 256);

#endif

#if defined(__AVX512F__)

GENERATE_SIMD_INTERNAL_TYPE(int, 512);

CREATE_INT512_OPERATOR_ASSIGN(8);
CREATE_INT512_OPERATOR_ASSIGN(16);
CREATE_INT512_OPERATOR_ASSIGN(32);
CREATE_INT512_OPERATOR_ASSIGN(64);
CREATE_INT512_OPERATOR_PLUS(8);
CREATE_INT512_OPERATOR_PLUS(16);
CREATE_INT512_OPERATOR_PLUS(32);
CREATE_INT512_OPERATOR_PLUS(64);
CREATE_INT512_OPERATOR_MINUS(8);
CREATE_INT512_OPERATOR_MINUS(16);
CREATE_INT512_OPERATOR_MINUS(32);
CREATE_INT512_OPERATOR_MINUS(64);

DECLARE_SIMD_USE_TYPE_INT(int, 512);


GENERATE_SIMD_INTERNAL_TYPE(float, 512);

CREATE_FLOAT_OPERATOR_ASSIGN(512);
CREATE_FLOAT_OPERATOR_PLUS(512);
CREATE_FLOAT_OPERATOR_MINUS(512);
CREATE_FLOAT_OPERATOR_MULTIPLY(512);

DECLARE_SIMD_USE_TYPE(float, 512);


GENERATE_SIMD_INTERNAL_TYPE(double, 512);

CREATE_DOUBLE_OPERATOR_ASSIGN(512);
CREATE_DOUBLE_OPERATOR_PLUS(512);
CREATE_DOUBLE_OPERATOR_MINUS(512);

DECLARE_SIMD_USE_TYPE(double, 512);

#endif

//SIMD::int_XXX checks are not ideal...
template<typename T>
using IsSIMDType = typename std::enable_if<
#if defined(__SSE2__) || defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
    (std::is_same<int, typename T::Type>::value && (T::BitWidth == 128) && IsElementAnyOfInts<typename T::ElementType>::value) || ( std::is_same<int, typename T::Type>::value && (T::BitWidth == 256) && IsElementAnyOfInts<typename T::ElementType>::value) ||
#endif
#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
    std::is_same<T, SIMD::float_256>::value || std::is_same<T, SIMD::double_256>::value ||
#endif
#if defined(__AVX512F__)
    ( std::is_same<int, typename T::Type>::value && (T::BitWidth == 512) && IsElementAnyOfInts<typename T::ElementType>::value) || std::is_same<T, SIMD::float_512>::value || std::is_same<T, SIMD::double_512>::value ||
#endif
    std::is_same<int,float>::value, //dummy
    int>::type;

namespace SIMD
{
template<typename T, unsigned int Length, IsSIMDType<T> = 0>
class Array
{
public:
    Array() : Data(nullptr)
    {
        void* vptr = static_cast<void*>(Data);
        allocate_aligned(vptr, T::SizeBytes * Length, T::Alignment);
        Data = static_cast<typename T::ElementType*>(vptr);
    }
    
    Array(const Array& other) : Data(nullptr)
    {
        void* vptr = static_cast<void*>(Data);
        allocate_aligned(vptr, T::SizeBytes * Length, T::Alignment);
        Data = static_cast<typename T::ElementType*>(vptr);
        for (unsigned int i = 0; i < Length; i++)
        {
            //Use raw data copy to avoid constructor calls
            memcpy(static_cast<char*>(Data) + i * T::SizeBytes, static_cast<char*>(other.Data) + i * T::SizeBytes, T::SizeBytes);
        }

    }

    Array(Array&& other) noexcept : Data(other.Data)
    {
        other.Data = nullptr;
    }
    Array& operator=(Array&& other)
    {
        if (this != &other)
        {
            Data = other.Data;
            other.Data = nullptr;
        }
        return *this;
    }

    Array& operator=(const Array& other)
    {
        memcpy((void*)Data, (void*)other.Data, T::SizeBytes * Length);
        return *this;
    }

    //Add + and - operators
    friend void operator+=(Array& lhs, const Array& rhs)
    {
        for (unsigned int i = 0; i < Length; i++)
        {
            T::AddInplaceRaw(lhs.Data + i*T::ElementCount, rhs.Data + i*T::ElementCount);
        }
    }

    friend void operator-=(Array& lhs, const Array& rhs)
    {
        for (unsigned int i = 0; i < Length; i++)
        {
            T::SubtractInplaceRaw(lhs.Data + i*T::ElementCount, rhs.Data + i*T::ElementCount);            
        }
    }

    friend void operator*=(Array& lhs, const Array& rhs)
    {
        for (unsigned int i = 0; i < Length; i++)
        {
            T::MultiplyInplaceRaw(lhs.Data + i*T::ElementCount, rhs.Data + i*T::ElementCount);
        }
    }

    _SIMD_INL_ typename T::ElementType* operator[](unsigned int index)
    {
        return Data + index*T::ElementCount;
    }

    static constexpr unsigned int Length = Length;
private:
    typename T::ElementType* Data;
};
}

#undef _SIMD_INL_
#undef GENERATE_SIMD
#undef INTERNAL_SIMD_TYPE_NAME
#undef CREATE_INT128_OPERATOR_PLUS
#undef CREATE_INT256_OPERATOR_PLUS
#undef CREATE_INT512_OPERATOR_PLUS
#undef CREATE_INT128_OPERATOR_MINUS
#undef CREATE_INT256_OPERATOR_MINUS
#undef CREATE_INT512_OPERATOR_MINUS
#undef GET_CPU_INFO
