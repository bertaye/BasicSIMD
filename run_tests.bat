cmake -S . -B build
cmake --build build\ --config Release
.\build\Release\BasicSIMD_Tests.exe > test_results.txt

python3 analyze_benchmarks.py --input_file=test_results.txt --output_dir=benchmark_results_windows_msvc/