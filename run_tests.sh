#!/bin/bash

# Configure the build using CMake
cmake -S . -B build

# Build the project with Release configuration
cmake --build build --config Release

# Run the tests
./build/BasicSIMD_Tests

# Make the output more readable
echo "Test execution complete."