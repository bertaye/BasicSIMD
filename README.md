# SIMD Operations Framework

A high-performance C++ framework for SIMD (Single Instruction Multiple Data) operations, providing optimized vector math operations for both floating-point and integer data types.

## Features

- Optimized SIMD operations for different data types:
  - `Int128` and `Int256` for integer operations
  - `Float256` for floating-point operations
- Standard mathematical operations:
  - Addition
  - Subtraction
  - Multiplication
- Automatic vectorization with significant performance improvements
- Comprehensive test suite using Google Test
- Performance benchmarks using Google Benchmark

## Performance Results

### Unit Tests Status

✅ **All Tests Passed**: 9 out of 9 tests passed successfully

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

### Performance Benchmarks

Performance improvements comparing SIMD operations vs. standard operations:

#### Float256 Operations (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.064 ms | 0.257 ms | 4.01x |
| Subtraction | 0.064 ms | 0.256 ms | 4.00x |
| Multiplication | 0.063 ms | 0.269 ms | 4.27x |

#### Int128 Operations (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.043 ms | 0.128 ms | 2.98x |
| Subtraction | 0.045 ms | 0.125 ms | 2.78x |
| Multiplication | 0.042 ms | 0.124 ms | 2.95x |

#### Int256 Operations (100,000 elements)

| Operation | SIMD Time | Plain Time | Speedup Factor |
|-----------|-----------|------------|----------------|
| Addition | 0.065 ms | 0.257 ms | 3.95x |
| Subtraction | 0.064 ms | 0.253 ms | 3.95x |
| Multiplication | 0.064 ms | 0.414 ms | 6.47x |

### Visual Performance Comparison

```
Performance Comparison (Lower is better)
-----------------------------------------------------------------
Float256 Addition     | SIMD [====                ] 0.064 ms
                      | Plain [================    ] 0.257 ms

Float256 Subtraction  | SIMD [====                ] 0.064 ms
                      | Plain [================    ] 0.256 ms

Float256 Multiplication| SIMD [====                ] 0.063 ms
                      | Plain [=================   ] 0.269 ms

Int128 Addition       | SIMD [===                 ] 0.043 ms
                      | Plain [========            ] 0.128 ms

Int128 Subtraction    | SIMD [===                 ] 0.045 ms
                      | Plain [========            ] 0.125 ms

Int128 Multiplication | SIMD [===                 ] 0.042 ms
                      | Plain [========            ] 0.124 ms

Int256 Addition       | SIMD [====                ] 0.065 ms
                      | Plain [================    ] 0.257 ms

Int256 Subtraction    | SIMD [====                ] 0.064 ms
                      | Plain [================    ] 0.253 ms

Int256 Multiplication | SIMD [====                ] 0.064 ms
                      | Plain [==========================] 0.414 ms
-----------------------------------------------------------------
```

### Speedup Factor Comparison

```
Speedup Factors (Higher is better)
-----------------------------------------------------------------
Int256 Multiplication | [==============================] 6.47x
Float256 Multiplication | [===================] 4.27x
Float256 Addition | [===================] 4.01x
Float256 Subtraction | [==================] 4.00x
Int256 Addition | [==================] 3.95x
Int256 Subtraction | [==================] 3.95x
Int128 Addition | [==============] 2.98x
Int128 Multiplication | [==============] 2.95x
Int128 Subtraction | [=============] 2.78x
-----------------------------------------------------------------
```

## Performance Analysis

The benchmark results demonstrate significant performance improvements when using SIMD operations compared to standard operations:

1. **Int256 Multiplication**: Shows the highest performance gain with a **6.47x speedup**, making it the most optimized operation.

2. **Float256 Operations**: Consistently show approximately **4x speedup** across addition, subtraction, and multiplication operations.

3. **Int128 Operations**: Show nearly **3x speedup** for all operations, which is still substantial but less than the wider 256-bit variants.

These results clearly demonstrate that SIMD operations provide significant performance benefits, especially for wider data types and multiplication operations.

## System Information

Tests were run on the following system:

- **CPU**: 16 cores @ 3294 MHz
- **Cache**:
  - L1 Data: 32 KiB (x8)
  - L1 Instruction: 32 KiB (x8)
  - L2 Unified: 512 KiB (x8)
  - L3 Unified: 16384 KiB (x1)
- **Test Date**: May 5, 2025

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

```bash
run_tests.bat
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the project
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request