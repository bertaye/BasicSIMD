# SIMD Operations Framework

A high-performance C++ framework for SIMD (Single Instruction Multiple Data) operations, providing optimized vector math operations for both floating-point and integer data types.

![SIMD Banner](https://user-images.githubusercontent.com/your-username/repository-name/banner.png)

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

### Visualization

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

## Performance Chart

![SIMD vs Plain Operations](https://quickchart.io/chart?c={type:'bar',data:{labels:['Float256 Add','Float256 Sub','Float256 Mul','Int128 Add','Int128 Sub','Int128 Mul','Int256 Add','Int256 Sub','Int256 Mul'],datasets:[{label:'SIMD (ms)',backgroundColor:'rgba(54,162,235,0.8)',data:[0.064,0.064,0.063,0.043,0.045,0.042,0.065,0.064,0.064]},{label:'Plain (ms)',backgroundColor:'rgba(255,99,132,0.8)',data:[0.257,0.256,0.269,0.128,0.125,0.124,0.257,0.253,0.414]}]},options:{title:{display:true,text:'SIMD vs Plain Operation Performance (Lower is Better)'},scales:{yAxes:[{ticks:{beginAtZero:true}}]},plugins:{datalabels:{align:'end',anchor:'end',formatter:function(value){return value+'ms'}}}}})

## Speedup Factor Chart

![Speedup Factors](https://quickchart.io/chart?c={type:'horizontalBar',data:{labels:['Int256 Mul','Float256 Mul','Float256 Add','Int256 Add','Int256 Sub','Float256 Sub','Int128 Add','Int128 Mul','Int128 Sub'],datasets:[{label:'Speedup Factor',backgroundColor:['rgba(255,99,132,0.8)','rgba(255,159,64,0.8)','rgba(255,205,86,0.8)','rgba(75,192,192,0.8)','rgba(54,162,235,0.8)','rgba(153,102,255,0.8)','rgba(201,203,207,0.8)','rgba(255,99,132,0.5)','rgba(255,159,64,0.5)'],data:[6.47,4.27,4.01,3.95,3.95,4.00,2.98,2.95,2.78]}]},options:{title:{display:true,text:'SIMD Speedup Factors (Higher is Better)'},plugins:{datalabels:{align:'end',anchor:'end',formatter:function(value){return value+'x'}}}}})

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

- CMake 3.10 or higher
- C++ compiler with SIMD support (AVX2 recommended)
- Git

### Building the Project

```bash
# Clone the repository
git clone https://github.com/yourusername/simd-framework.git
cd simd-framework

# Build the project
cmake -S . -B build
cmake --build build --config Release
```

### Running Tests

```bash
# Run all tests and benchmarks
./build/Release/BasicSIMD_Tests.exe
```

Or use the provided batch script:

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