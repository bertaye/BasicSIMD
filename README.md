# SIMD Operations Framework

A high-performance C++ framework for SIMD (Single Instruction Multiple Data) operations, providing optimized vector math operations for both floating-point and integer data types.

## Features

- Optimized SIMD operations for different data types:
  - `Int128`, `Int256` and `Int512` for integer operations (with `int8_t`, `int16_t`, and `int32_t`)
  - `Float256` and `Float512` for floating-point operations
  - `Double256` and `Double512` for double-precision operations
- Standard mathematical operations:
  - Addition
  - Subtraction
  - Multiplication
  - Division (for floating-point and double-precision)
- Automatic vectorization with significant performance improvements
- Comprehensive test suite using Google Test
- Performance benchmarks using Google Benchmark

## Performance Results

### Unit Tests Status

✅ **All Tests Passed**: 10 out of 10 tests passed successfully

| Test | Status | Time |
|------|--------|------|
| int128_Addition | ✓ PASS | 0 ms |
| int128_Subtraction | ✓ PASS | 0 ms |
| int128_Multiplication | ✓ PASS | 0 ms |
| int256_Addition | ✓ PASS | 1 ms |
| int256_Subtraction | ✓ PASS | 1 ms |
| int256_Multiplication | ✓ PASS | 1 ms |
| float256_Addition | ✓ PASS | 1 ms |
| float256_Subtraction | ✓ PASS | 1 ms |
| float256_Multiplication | ✓ PASS | 1 ms |
| float256_Division | ✓ PASS | 1 ms |

### Performance Benchmarks

Performance improvements comparing SIMD operations vs. standard operations:

#### Float256 Operations (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.069 ms | 0.273 ms | 3.96x |
| Subtraction | 0.066 ms | 0.270 ms | 4.09x |
| Multiplication | 0.072 ms | 0.292 ms | 4.06x |
| Division | 0.084 ms | 0.701 ms | 8.35x |

#### Double256 Operations (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.064 ms | 0.146 ms | 2.28x |
| Subtraction | 0.071 ms | 0.151 ms | 2.13x |
| Multiplication | 0.077 ms | 0.213 ms | 2.77x |
| Division | 0.130 ms | 0.464 ms | 3.57x |

#### Int128 Operations with int32_t (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.051 ms | 0.134 ms | 2.63x |
| Subtraction | 0.063 ms | 0.140 ms | 2.22x |
| Multiplication | 0.072 ms | 0.196 ms | 2.72x |

#### Int128 Operations with int16_t (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.055 ms | 0.285 ms | 5.18x |
| Subtraction | 0.046 ms | 0.267 ms | 5.80x |
| Multiplication | 0.044 ms | 0.444 ms | 10.09x |

#### Int128 Operations with int8_t (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.046 ms | 0.517 ms | 11.24x |
| Subtraction | 0.046 ms | 0.507 ms | 11.02x |

#### Int256 Operations with int32_t (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.070 ms | 0.273 ms | 3.90x |
| Subtraction | 0.065 ms | 0.277 ms | 4.26x |
| Multiplication | 0.070 ms | 0.269 ms | 3.84x |

#### Int256 Operations with int16_t (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.076 ms | 0.622 ms | 8.18x |
| Subtraction | 0.067 ms | 0.519 ms | 7.75x |
| Multiplication | 0.064 ms | 0.870 ms | 13.59x |

#### Int256 Operations with int8_t (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.067 ms | 1.02 ms | 15.22x |
| Subtraction | 0.071 ms | 1.06 ms | 14.93x |

### Speedup Factor Comparison

```
Speedup Factors (Higher is better)
-------------------------------------------------------------
Int256 int8_t Addition       | 🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵⚪ | 15.22x |
Int256 int8_t Subtraction    | 🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪ | 14.93x |
Int256 int16_t Multiplication| 🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪ | 13.59x |
Int128 int8_t Addition       | 🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪ | 11.24x |
Int128 int8_t Subtraction    | 🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪ | 11.02x |
Int128 int16_t Multiplication| 🔵🔵🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪ | 10.09x |
Float256 Division            | 🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪ | 8.35x |
Int256 int16_t Addition      | 🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪ | 8.18x |
Int256 int16_t Subtraction   | 🔵🔵🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪ | 7.75x |
Int128 int16_t Subtraction   | 🔵🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 5.80x |
Int128 int16_t Addition      | 🔵🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 5.18x |
Int256 int32_t Subtraction   | 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 4.26x |
Float256 Subtraction         | 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 4.09x |
Float256 Multiplication      | 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 4.06x |
Float256 Addition            | 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 3.96x |
Int256 int32_t Addition      | 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 3.90x |
Int256 int32_t Multiplication| 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 3.84x |
Double256 Division           | 🔵🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 3.57x |
Double256 Multiplication     | 🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 2.77x |
Int128 int32_t Multiplication| 🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 2.72x |
Int128 int32_t Addition      | 🔵🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 2.63x |
Double256 Addition           | 🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 2.28x |
Int128 int32_t Subtraction   | 🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 2.22x |
Double256 Subtraction        | 🔵🔵⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪⚪ | 2.13x |
-------------------------------------------------------------
```

## System Information

Tests and benchmarks were run on the following system:

- **CPU**: 16 cores @ 3294 MHz
- **Cache**:
  - L1 Data: 32 KiB (x8)
  - L1 Instruction: 32 KiB (x8)
  - L2 Unified: 512 KiB (x8)
  - L3 Unified: 16384 KiB (x1)
- **Test Date**: May 6, 2025

## Getting Started

### Prerequisites

- C++ compiler with SIMD support (AVX2 recommended)

### Using the Project

```bash
This is a header only project, no need for building. 
#include<SIMD.h>
Should be sufficient.
```

### Running Tests & Benchmarks

Use the provided batch script:

 #### Windows
```bash
run_tests.bat
```

#### Linux
```bash
run_tests.sh
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Current state of the project is purely based on the personal needs, any contribution for extending SIMD support is appreciated & welcome.

The SIMD.h uses macros a lot to eliminate manual work of repetitive coding, but the idea is really simple; defining a macro for a new operator/function as a specialization of base SIMD_Type_t and then using it for different bit widths.