#!/bin/bash

# Configure the build using CMake
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build

# Build the project with Release configuration
cmake --build build --config Release

# Run the tests
./build/BasicSIMD_Tests > test_results.txt

# Generate benchmark analysis
python3 analyze_benchmarks.py --input_file=test_results.txt --output_dir=benchmark_results_linux_gcc/

# Make the output more readable
echo "Test execution completed, plots saved to benchmark_results/ directory and README.md updated."