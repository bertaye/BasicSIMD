cmake -S . -B build
cmake --build build\ --config Release
.\build\Release\BasicSIMD_Tests.exe > test_results_windows.txt

python analyze_benchmarks.py --input_file=test_results_windows.txt --output_dir=benchmark_results_windows_msvc/