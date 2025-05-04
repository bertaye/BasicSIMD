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

namespace AlignedMemory
{

template<typename T>
class AlignedDeleter {
public:
    explicit AlignedDeleter(size_t alignment = 32) : alignment_(alignment) {}
    
    void operator()(T* ptr) const {
        #ifdef _WIN32
            _aligned_free(ptr);
        #else
            free(ptr);
        #endif
    }
    
private:
    size_t alignment_;
};

template<typename T>
class AlignedAllocator {
public:
    using pointer = T*;
    using size_type = std::size_t;
    
    explicit AlignedAllocator(size_t alignment = 32) : alignment_(alignment) {}
    
    pointer allocate(size_type n) {
        #ifdef _WIN32
            data = _aligned_malloc(n * sizeof(T), alignment_);
        #else
            data = aligned_alloc(alignment_, n * sizeof(T));
        #endif
        
        if (!data) throw std::bad_alloc();
        return static_cast<pointer>(data);
    }
    void* get() const {
        return data;
    }
private:
    void* data;
    size_t alignment_;
};

template<typename T>
using AlignedPtr = std::unique_ptr<T, AlignedDeleter<T>>;

template<typename T>
AlignedPtr<T> make_aligned(size_t size, size_t alignment = 32) {
    AlignedAllocator<T> allocator(alignment);
    AlignedDeleter<T> deleter(alignment);
    return AlignedPtr<T>(allocator.allocate(size), deleter);
}

}

#ifdef _MSC_VER
    #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
    #include <cpuid.h>
    #include <x86intrin.h>
#endif

enum class InstructionSet {
    NONE = 0,
    SSE = 1,
    SSE2 = 2,  // Added SSE2
    AVX = 3,   // Updated ordinal values
    AVX2 = 4,  // Updated ordinal values
    AVX512 = 5 // Updated ordinal values
};

class CPUFeatures {
private:
    static bool initialized_;
    static bool has_sse_;
    static bool has_sse2_;
    static bool has_avx_;
    static bool has_avx2_;
    static bool has_avx512f_;


    static void initialize() {
        if (initialized_) return;

        #if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
            std::array<int, 4> cpui{};
            
            // Get vendor string and max CPUID level
            #if defined(_MSC_VER)
                __cpuid(cpui.data(), 0);
            #else
                unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
                __get_cpuid(0, &eax, &ebx, &ecx, &edx);
                cpui = {static_cast<int>(eax), static_cast<int>(ebx), 
                        static_cast<int>(ecx), static_cast<int>(edx)};
            #endif
            
            int max_std_id = cpui[0];
            
            // Check SSE, SSE2 and AVX
            if (max_std_id >= 1) {
                #if defined(_MSC_VER)
                    __cpuid(cpui.data(), 1);
                #else
                    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
                    cpui = {static_cast<int>(eax), static_cast<int>(ebx), 
                            static_cast<int>(ecx), static_cast<int>(edx)};
                #endif
                
                // Raw CPU feature flags
                bool cpu_has_sse = (cpui[3] & (1 << 25)) != 0;      // EDX bit 25
                bool cpu_has_sse2 = (cpui[3] & (1 << 26)) != 0;     // EDX bit 26
                bool cpu_has_avx = (cpui[2] & (1 << 28)) != 0;      // ECX bit 28
                bool cpu_uses_xsave = (cpui[2] & (1 << 27)) != 0;   // ECX bit 27
                
                // Safely assign SSE/SSE2 flags (these don't need OS support)
                has_sse_ = cpu_has_sse;
                has_sse2_ = cpu_has_sse2;
                
                // For AVX and beyond, we need to check OS support via XCR0
                if (cpu_has_avx && cpu_uses_xsave) {
                    // Get XCR0 register to check OS support for YMM/ZMM state saving
                    unsigned long long xcrFeatureMask = 0;
                    try {
                        #if defined(_MSC_VER)
                            xcrFeatureMask = _xgetbv(0);
                        #elif defined(__GNUC__) || defined(__clang__)
                            unsigned int eax_xcr = 0, edx_xcr = 0;
                            __asm__ __volatile__("xgetbv" : "=a"(eax_xcr), "=d"(edx_xcr) : "c"(0));
                            xcrFeatureMask = ((unsigned long long)edx_xcr << 32) | eax_xcr;
                        #endif
                        
                        // Check bits 1 and 2 for SSE and AVX state saving (XMM and YMM registers)
                        // XCR0[2:1] = '11b' (XMM and YMM are enabled by OS)
                        bool avxSupportedByOS = (xcrFeatureMask & 0x6) == 0x6;
                        
                        has_avx_ = cpu_has_avx && avxSupportedByOS;
                        
                        // Check AVX2 and AVX-512
                        if (max_std_id >= 7) {
                            #if defined(_MSC_VER)
                                __cpuidex(cpui.data(), 7, 0);
                            #else
                                __cpuid_count(7, 0, eax, ebx, ecx, edx);
                                cpui = {static_cast<int>(eax), static_cast<int>(ebx), 
                                        static_cast<int>(ecx), static_cast<int>(edx)};
                            #endif
                            
                            bool cpu_has_avx2 = (cpui[1] & (1 << 5)) != 0;         // EBX bit 5
                            bool cpu_has_avx512f = (cpui[1] & (1 << 16)) != 0;     // EBX bit 16
                            
                            has_avx2_ = cpu_has_avx2 && avxSupportedByOS;
                            
                            // For AVX-512, additionally check bits 7:5 of XCR0 for ZMM state
                            // XCR0[7:5] = '111b' (ZMM 0-15, ZMM 16-31 and mask registers)
                            bool avx512SupportedByOS = avxSupportedByOS && 
                                                     ((xcrFeatureMask & 0xE0) == 0xE0);
                            
                            has_avx512f_ = cpu_has_avx512f && avx512SupportedByOS;
                        }
                    } catch (...) {
                        has_avx_ = false;
                        has_avx2_ = false;
                        has_avx512f_ = false;
                    }
                } else {
                    has_avx_ = false;
                    has_avx2_ = false;
                    has_avx512f_ = false;
                }
            }
        #else
            has_sse_ = false;
            has_sse2_ = false;
            has_avx_ = false;
            has_avx2_ = false;
            has_avx512f_ = false;
        #endif

        initialized_ = true;
        
    }

public:
    static bool hasSSE() {
        if (!initialized_) initialize();
        return has_sse_;
    }

    static bool hasSSE2() {
        if (!initialized_) initialize();
        return has_sse2_;
    }

    static bool hasAVX() {
        if (!initialized_) initialize();
        return has_avx_;
    }

    static bool hasAVX2() {
        if (!initialized_) initialize();
        return has_avx2_;
    }

    static bool hasAVX512() {
        if (!initialized_) initialize();
        return has_avx512f_;
    }

    template<InstructionSet T>
    static typename std::enable_if<T == InstructionSet::NONE, bool>::type 
    supportsInstructionSet() {
        return true;  // NONE is always supported
    }

    template<InstructionSet T>
    static typename std::enable_if<T == InstructionSet::SSE, bool>::type
    supportsInstructionSet() {
        return hasSSE();
    }

    template<InstructionSet T>
    static typename std::enable_if<T == InstructionSet::SSE2, bool>::type
    supportsInstructionSet() {
        return hasSSE2();
    }

    template<InstructionSet T>
    static typename std::enable_if<T == InstructionSet::AVX, bool>::type
    supportsInstructionSet() {
        return hasAVX();
    }

    template<InstructionSet T>
    static typename std::enable_if<T == InstructionSet::AVX2, bool>::type
    supportsInstructionSet() {
        return hasAVX2();
    }

    template<InstructionSet T>
    static typename std::enable_if<T == InstructionSet::AVX512, bool>::type
    supportsInstructionSet() {
        return hasAVX512();
    }

    // New function to print all supported instruction sets
    static void printSupportedInstructionSets() {
        if (!initialized_) initialize();
        
        std::cout << "Supported SIMD Instruction Sets:" << std::endl;
        std::cout << "-------------------------------" << std::endl;
        std::cout << "SSE:    " << (has_sse_ ? "Yes" : "No") << std::endl;
        std::cout << "SSE2:   " << (has_sse2_ ? "Yes" : "No") << std::endl;
        std::cout << "AVX:    " << (has_avx_ ? "Yes" : "No") << std::endl;
        std::cout << "AVX2:   " << (has_avx2_ ? "Yes" : "No") << std::endl;
        std::cout << "AVX512: " << (has_avx512f_ ? "Yes" : "No") << std::endl;
    }
};

// Initialize static members
bool CPUFeatures::initialized_ = false;
bool CPUFeatures::has_sse_ = false;
bool CPUFeatures::has_sse2_ = false;  // Initialize SSE2 static member
bool CPUFeatures::has_avx_ = false;
bool CPUFeatures::has_avx2_ = false;
bool CPUFeatures::has_avx512f_ = false;

// Primary template - default is NONE
template<typename T, size_t BitWidth>
struct RequiredInstructionSet {
    static constexpr InstructionSet value = InstructionSet::NONE;
};

// Macro for specializations
#define DEFINE_REQUIRED_INSTRUCTION_SET(TYPE, BITS, SET) \
template<> \
struct RequiredInstructionSet<TYPE, BITS> { \
    static constexpr InstructionSet value = ##InstructionSet::SET; \
    static constexpr const char* name = #SET; \
};

// Integer types
DEFINE_REQUIRED_INSTRUCTION_SET(int8_t, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(uint8_t, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(int16_t, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(uint16_t, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(int32_t, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(uint32_t, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(int64_t, 128, SSE2)
DEFINE_REQUIRED_INSTRUCTION_SET(uint64_t, 128, SSE2)

DEFINE_REQUIRED_INSTRUCTION_SET(int8_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(uint8_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(int16_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(uint16_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(int32_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(uint32_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(int64_t, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(uint64_t, 256, AVX)

DEFINE_REQUIRED_INSTRUCTION_SET(int8_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(uint8_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(int16_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(uint16_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(int32_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(uint32_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(int64_t, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(uint64_t, 512, AVX512)

// Floating-point types
DEFINE_REQUIRED_INSTRUCTION_SET(float, 128, SSE)
DEFINE_REQUIRED_INSTRUCTION_SET(double, 128, SSE2)

DEFINE_REQUIRED_INSTRUCTION_SET(float, 256, AVX)
DEFINE_REQUIRED_INSTRUCTION_SET(double, 256, AVX)

DEFINE_REQUIRED_INSTRUCTION_SET(float, 512, AVX512)
DEFINE_REQUIRED_INSTRUCTION_SET(double, 512, AVX512)

#undef DEFINE_REQUIRED_INSTRUCTION_SET


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

template<typename ContainerType, int Bits, typename T_ElementType, 
typename = IsElementValid<ContainerType, T_ElementType>,
typename = typename std::enable_if<Bits%8 == 0, int>::type >
struct SIMD_Type_t
{
public:
    using Type = typename ContainerType;
    using ElementType = typename T_ElementType;
    static constexpr unsigned int BitWidth = Bits;
    static constexpr unsigned int SizeBytes = Bits/8;
    static constexpr unsigned int Alignment = Bits/8;
    static constexpr unsigned int ElementCount = (Bits/8)/sizeof(T_ElementType);
    void* Data;
    AlignedMemory::AlignedPtr<T_ElementType> AlignedData;
private:
    bool IsImported;

public:
    SIMD_Type_t() :Data(nullptr), IsImported(false)
    {
		if (!CPUFeatures::supportsInstructionSet<RequiredInstructionSet<ContainerType, Bits>::value>())
		{
            std::string errorMessage = "For SIMD Type " + std::string(typeid(ContainerType).name()) + "_" + std::to_string(Bits) + " the required instruction set " + std::string(RequiredInstructionSet<ContainerType, Bits>::name) + " does not supported in this device.";
            throw std::runtime_error(errorMessage);
		}
        AlignedData = std::move(AlignedMemory::make_aligned<T_ElementType>(SizeBytes, Alignment));
        Data = AlignedData.get();
    }
    template<typename ...Args,
                    IsElementValid<ContainerType, T_ElementType> = 0, 
                    IsAllElementsCompatible<T_ElementType, Args...> = 0, 
                    IsSizeValid< SizeBytes / sizeof(T_ElementType), Args...> = 0>
    SIMD_Type_t(Args... args) : Data(nullptr), IsImported(false) 
    {
        if (!CPUFeatures::supportsInstructionSet<RequiredInstructionSet<ContainerType, Bits>::value>())
        {
            std::string errorMessage = "For SIMD Type " + std::string(typeid(ContainerType).name()) + "_" + std::to_string(Bits) + " the required instruction set " + std::string(RequiredInstructionSet<ContainerType, Bits>::name) + " does not supported in this device.";
            throw std::runtime_error(errorMessage);
        }
        alignas(Alignment) T_ElementType data[SizeBytes / sizeof(T_ElementType)] = { args... };
        AlignedData = std::move(AlignedMemory::make_aligned<T_ElementType>(SizeBytes, Alignment));
        Data = AlignedData.get();
        std::copy(data, data + SizeBytes / sizeof(T_ElementType), reinterpret_cast<T_ElementType*>(Data));
    }
    SIMD_Type_t(void* data) : Data(nullptr)
    {
        if(!CPUFeatures::supportsInstructionSet<RequiredInstructionSet<ContainerType, Bits>::value>())
        {
            std::string errorMessage = "For SIMD Type " + std::string(typeid(ContainerType).name()) + "_" + std::to_string(Bits) + " the required instruction set " + std::string(RequiredInstructionSet<ContainerType, Bits>::name) + " does not supported in this device.";
            throw std::runtime_error(errorMessage);
        }
        /*This will check for memory alignment*/
        if((reinterpret_cast<uintptr_t>(data) & (Alignment - 1)) != 0)
        {
            throw std::runtime_error("Data is not aligned to the required boundary.");
        }
        IsImported = true;
        Data = data;
    }
    /* Move constructor */ 
    SIMD_Type_t(SIMD_Type_t&& other) noexcept : Data(other.Data), IsImported(other.IsImported) { 
        other.Data = nullptr; 
        other.IsImported = false;
    } 
    SIMD_Type_t(const SIMD_Type_t& other) noexcept : Data(nullptr), IsImported(false) { 
        AlignedData = std::move(AlignedMemory::make_aligned<T_ElementType>(SizeBytes, Alignment));
        Data = AlignedData.get();
        std::copy(other.Data, other.Data + SizeBytes / sizeof(T_ElementType), reinterpret_cast<T_ElementType*>(Data));
    } 
    ~SIMD_Type_t()
    {
    }
    static _SIMD_INL_ SIMD_Type_t Import(void* data)
    {
        return std::move(SIMD_Type_t(data));
    }
    const T_ElementType *const Get()
    {
        return reinterpret_cast<T_ElementType*>(Data);
    }
    /* Copy assignment operator */ 
    SIMD_Type_t& operator=(const SIMD_Type_t& other) { 
        Data = other.Data;\
        return *this; \
    } 
    explicit operator bool() noexcept { 
        return Data != nullptr; 
    }
    
    T_ElementType ElementAt(unsigned int index) const
	{
        if(index >= ElementCount)
		{
			return 0;
		}
		return *(reinterpret_cast<T_ElementType*>(Data) + index);
	}
    static _SIMD_INL_ SIMD_Type_t Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {
        SIMD_Type_t result;
        /* Implementation will be specialized */
        return result;
    }
    static _SIMD_INL_ void AddInplace(SIMD_Type_t& to, const SIMD_Type_t& from) {
        /* Implementation will be specialized */
    }
    static _SIMD_INL_ void AddInplaceRaw(T_ElementType* to, const T_ElementType* from) {
        /* Implementation will be specialized */
    }
    
    static _SIMD_INL_ SIMD_Type_t Subtract(const SIMD_Type_t& a, const SIMD_Type_t& b) {
        SIMD_Type_t result;
        /* Implementation will be specialized */
        return result;
    }
    static _SIMD_INL_ void SubtractInplace(SIMD_Type_t& to, const SIMD_Type_t& from) {
        /* Implementation will be specialized */
    }
    static _SIMD_INL_ void SubtractInplaceRaw(T_ElementType* to, const T_ElementType* from) {
        /* Implementation will be specialized */
    }
    
    static _SIMD_INL_ SIMD_Type_t Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {
        SIMD_Type_t result;
        /* Implementation will be specialized */
        return result;
    }
    static _SIMD_INL_ void MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from) {
        /* Implementation will be specialized */
    }
    static _SIMD_INL_ void MultiplyInplaceRaw(T_ElementType* to, const T_ElementType* from) {
        /* Implementation will be specialized */
    }
    
    _SIMD_INL_ SIMD_Type_t operator+(const SIMD_Type_t& other) const
    {
        return Add(*this, other);
    }
    _SIMD_INL_ SIMD_Type_t operator*(const SIMD_Type_t& other) const
    {
        return Multiply(*this, other);
    }
    _SIMD_INL_ SIMD_Type_t operator-(const SIMD_Type_t& other) const
    {
        return Subtract(*this, other);
    }
    _SIMD_INL_ void operator+=(const SIMD_Type_t& other)
    {
        AddInplace(*this, other);
    }
    _SIMD_INL_ void operator*=(const SIMD_Type_t& other)
    {
        MultiplyInplace(*this, other);
    }
    void operator-=(const SIMD_Type_t& other)
    {
        SubtractInplace(*this, other);
    }
    _SIMD_INL_ T_ElementType& operator[](unsigned int index)
    {
        return *(reinterpret_cast<T_ElementType*>(Data) + index);
    }
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


#define CREATE_INT128_OPERATOR_PLUS(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, int##XX##_t> SIMD_Type_t<int, 128, int##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t<int, 128, int##XX##_t> result;\
    _mm_store_si128((__m128i*)result.Data, _mm_add_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, uint##XX##_t> SIMD_Type_t<int, 128,uint##XX##_t>::Add(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t<int, 128, uint##XX##_t> result;\
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

#define CREATE_INT128_OPERATOR_MULTIPLY(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, int##XX##_t> SIMD_Type_t<int, 128, int##XX##_t>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm_store_si128((__m128i*)result.Data, _mm_mullo_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 128, uint##XX##_t> SIMD_Type_t<int, 128,uint##XX##_t>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm_store_si128((__m128i*)result.Data, _mm_mullo_epi##XX(_mm_load_si128((__m128i*)a.Data), _mm_load_si128((__m128i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, int##XX##_t>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm_store_si128((__m128i*)to.Data, _mm_mullo_epi##XX(_mm_load_si128((__m128i*)to.Data), _mm_load_si128((__m128i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, uint##XX##_t>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm_store_si128((__m128i*)to.Data, _mm_mullo_epi##XX(_mm_load_si128((__m128i*)to.Data), _mm_load_si128((__m128i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, int##XX##_t>::MultiplyInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm_store_si128((__m128i*)to, _mm_mullo_epi##XX(_mm_load_si128((__m128i*)to), _mm_load_si128((__m128i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 128, uint##XX##_t>::MultiplyInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm_store_si128((__m128i*)to, _mm_mullo_epi##XX(_mm_load_si128((__m128i*)to), _mm_load_si128((__m128i*)from)));\
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

#define CREATE_INT256_OPERATOR_MULTIPLY(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, int##XX##_t> SIMD_Type_t<int, 256, int##XX##_t>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm256_store_si256((__m256i*)result.Data, _mm256_mullo_epi##XX(_mm256_load_si256((__m256i*)a.Data), _mm256_load_si256((__m256i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 256, uint##XX##_t> SIMD_Type_t<int, 256,uint##XX##_t>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm256_store_si256((__m256i*)result.Data, _mm256_mullo_epi##XX(_mm256_load_si256((__m256i*)a.Data), _mm256_load_si256((__m256i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, int##XX##_t>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm256_store_si256((__m256i*)to.Data, _mm256_mullo_epi##XX(_mm256_load_si256((__m256i*)to.Data), _mm256_load_si256((__m256i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, uint##XX##_t>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm256_store_si256((__m256i*)to.Data, _mm256_mullo_epi##XX(_mm256_load_si256((__m256i*)to.Data), _mm256_load_si256((__m256i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, int##XX##_t>::MultiplyInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm256_store_si256((__m256i*)to, _mm256_mullo_epi##XX(_mm256_load_si256((__m256i*)to), _mm256_load_si256((__m256i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 256, uint##XX##_t>::MultiplyInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm256_store_si256((__m256i*)to, _mm256_mullo_epi##XX(_mm256_load_si256((__m256i*)to), _mm256_load_si256((__m256i*)from)));\
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

#define CREATE_INT512_OPERATOR_MULTIPLY(XX) \
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, int##XX##_t> SIMD_Type_t<int, 512, int##XX##_t>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm512_store_si512((__m512i*)result.Data, _mm512_mullo_epi##XX(_mm512_load_si512((__m512i*)a.Data), _mm512_load_si512((__m512i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ SIMD_Type_t<int, 512, uint##XX##_t> SIMD_Type_t<int, 512,uint##XX##_t>::Multiply(const SIMD_Type_t& a, const SIMD_Type_t& b) {\
    SIMD_Type_t result;\
    _mm512_store_si512((__m512i*)result.Data, _mm512_mullo_epi##XX(_mm512_load_si512((__m512i*)a.Data), _mm512_load_si512((__m512i*)b.Data)));\
    return result;\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, int##XX##_t>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm512_store_si512((__m512i*)to.Data, _mm512_mullo_epi##XX(_mm512_load_si512((__m512i*)to.Data), _mm512_load_si512((__m512i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, uint##XX##_t>::MultiplyInplace(SIMD_Type_t& to, const SIMD_Type_t& from)\
{\
    _mm512_store_si512((__m512i*)to.Data, _mm512_mullo_epi##XX(_mm512_load_si512((__m512i*)to.Data), _mm512_load_si512((__m512i*)from.Data)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, int##XX##_t>::MultiplyInplaceRaw(int##XX##_t* to, const int##XX##_t* from)\
{\
    _mm512_store_si512((__m512i*)to, _mm512_mullo_epi##XX(_mm512_load_si512((__m512i*)to), _mm512_load_si512((__m512i*)from)));\
}\
template<>\
_SIMD_INL_ void SIMD_Type_t<int, 512, uint##XX##_t>::MultiplyInplaceRaw(uint##XX##_t* to, const uint##XX##_t* from)\
{\
    _mm512_store_si512((__m512i*)to, _mm512_mullo_epi##XX(_mm512_load_si512((__m512i*)to), _mm512_load_si512((__m512i*)from)));\
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


#if defined(__SSE2__) || defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)


CREATE_INT128_OPERATOR_PLUS(8);
CREATE_INT128_OPERATOR_PLUS(16);
CREATE_INT128_OPERATOR_PLUS(32);
CREATE_INT128_OPERATOR_PLUS(64);
CREATE_INT128_OPERATOR_MINUS(8);
CREATE_INT128_OPERATOR_MINUS(16);
CREATE_INT128_OPERATOR_MINUS(32);
CREATE_INT128_OPERATOR_MINUS(64);
CREATE_INT128_OPERATOR_MULTIPLY(16);
CREATE_INT128_OPERATOR_MULTIPLY(32);
CREATE_INT128_OPERATOR_MULTIPLY(64);

DECLARE_SIMD_USE_TYPE_INT(int, 128);


CREATE_INT256_OPERATOR_PLUS(8);
CREATE_INT256_OPERATOR_PLUS(16);
CREATE_INT256_OPERATOR_PLUS(32);
CREATE_INT256_OPERATOR_PLUS(64);
CREATE_INT256_OPERATOR_MINUS(8);
CREATE_INT256_OPERATOR_MINUS(16);
CREATE_INT256_OPERATOR_MINUS(32);
CREATE_INT256_OPERATOR_MINUS(64);
CREATE_INT256_OPERATOR_MULTIPLY(16);
CREATE_INT256_OPERATOR_MULTIPLY(32);
CREATE_INT256_OPERATOR_MULTIPLY(64);


DECLARE_SIMD_USE_TYPE_INT(int, 256);

#endif

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)

CREATE_FLOAT_OPERATOR_PLUS(256);
CREATE_FLOAT_OPERATOR_MINUS(256);
CREATE_FLOAT_OPERATOR_MULTIPLY(256);

DECLARE_SIMD_USE_TYPE(float, 256);


CREATE_DOUBLE_OPERATOR_PLUS(256);
CREATE_DOUBLE_OPERATOR_MINUS(256);

DECLARE_SIMD_USE_TYPE(double, 256);

#endif

#if defined(__AVX512F__)


CREATE_INT512_OPERATOR_PLUS(8);
CREATE_INT512_OPERATOR_PLUS(16);
CREATE_INT512_OPERATOR_PLUS(32);
CREATE_INT512_OPERATOR_PLUS(64);
CREATE_INT512_OPERATOR_MINUS(8);
CREATE_INT512_OPERATOR_MINUS(16);
CREATE_INT512_OPERATOR_MINUS(32);
CREATE_INT512_OPERATOR_MINUS(64);
CREATE_INT512_OPERATOR_MULTIPLY(16);
CREATE_INT512_OPERATOR_MULTIPLY(32);
CREATE_INT512_OPERATOR_MULTIPLY(64);

DECLARE_SIMD_USE_TYPE_INT(int, 512);



CREATE_FLOAT_OPERATOR_PLUS(512);
CREATE_FLOAT_OPERATOR_MINUS(512);
CREATE_FLOAT_OPERATOR_MULTIPLY(512);

DECLARE_SIMD_USE_TYPE(float, 512);



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
template<typename T, unsigned int _Length, IsSIMDType<T> = 0>
class Array
{
public:
    Array() : Data(nullptr)
    {
        AlignedData = std::move(AlignedMemory::make_aligned<typename T::ElementType>(T::SizeBytes * Length, T::Alignment));
        Data = static_cast<typename T::ElementType*>(AlignedData.get());
    }
    
    Array(const Array& other) : Data(nullptr)
    {
        AlignedData = std::move(AlignedMemory::make_aligned<typename T::ElementType>(T::SizeBytes * Length, T::Alignment));
        Data = static_cast<typename T::ElementType*>(AlignedData.get());
        memcpy((void*)Data, (void*)other.Data, T::SizeBytes * Length);
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

    static constexpr unsigned int Length = _Length;
private:
    typename T::ElementType* Data;
    AlignedMemory::AlignedPtr<typename T::ElementType> AlignedData;
    
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
