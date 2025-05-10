import re
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path
import argparse  # Add argparse for command line arguments
import cpuinfo
import datetime  # Add datetime for generation timestamp
import platform
import subprocess

def parse_benchmark_results(file_path):
    """Parse benchmark results from file."""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Find the benchmark table
    pattern = r"^-+\nBenchmark\s+Time\s+CPU\s+Iterations\n-+\n([\s\S]+?)(?:\n\n|\Z)"
    match = re.search(pattern, content, re.MULTILINE)
    
    if not match:
        print("Benchmark results not found in the file.")
        return None
    
    benchmark_text = match.group(1)
    
    # Parse benchmark results
    results = []
    for line in benchmark_text.strip().split('\n'):
        if not line.strip():
            continue
        
        parts = line.split()
        if len(parts) < 4:  # Ensure we have enough parts
            print(f"Skipping invalid line: {line}")
            continue
            
        benchmark_name = parts[0]
        
        # Check if the time value is numeric
        try:
            time_value = parts[1]
            time_unit = parts[2]
            
            # Convert time to milliseconds
            if time_unit == 'ns':
                time_ms = float(time_value) / 1000000
            elif time_unit == 'us':
                time_ms = float(time_value) / 1000
            elif time_unit == 'ms':
                time_ms = float(time_value)
            elif time_unit == 's':
                time_ms = float(time_value) * 1000
            else:
                print(f"Skipping line with unknown time unit: {line}")
                continue
        except (ValueError, IndexError):
            print(f"Skipping line with invalid time format: {line}")
            continue
        
        # Extract components using regex for better accuracy
        name_match = re.match(r'BM_(SIMD|Plain)_([^_]+)(?:_with_([^_]+))?((?:_[^_]+){1,2})_(\d+)', benchmark_name)
        
        if name_match:
            category = name_match.group(1)  # SIMD or Plain
            base_type = name_match.group(2)  # float256, int128, etc.
            with_type = name_match.group(3)  # int32_t, int16_t, etc. or None
            operation_part = name_match.group(4).strip('_')  # Operation name
            size = name_match.group(5)  # Size value
            
            # Construct data_type
            if with_type:
                data_type = f"{base_type}_with_{with_type}"
            else:
                data_type = base_type
            
            # Handle operation part
            # Check if it ends with _t (from int32_t, etc.)
            if operation_part.endswith('_t'):
                # Split by underscore and take the first part as the operation
                op_parts = operation_part.split('_')
                if len(op_parts) > 1:
                    operation = op_parts[-1]  # Last part should be the operation
                else:
                    operation = operation_part
            else:
                operation = operation_part
        else:
            # Fallback for unrecognized formats - try a simpler pattern
            simple_match = re.match(r'BM_(SIMD|Plain)_([^_]+)_([^_]+)_(\d+)', benchmark_name)
            if simple_match:
                category = simple_match.group(1)
                data_type = simple_match.group(2)
                operation = simple_match.group(3)
                size = simple_match.group(4)
            else:
                # Last fallback
                print(f"Warning: Could not parse benchmark name: {benchmark_name}")
                category = "Unknown"
                data_type = "Unknown"
                operation = "Unknown" 
                size = "Unknown"
        
        results.append({
            'name': benchmark_name,
            'category': category,
            'data_type': data_type,
            'operation': operation,
            'size': size,
            'time_ms': time_ms
        })
    
    # Debug output to check what benchmarks were parsed
    result_df = pd.DataFrame(results)
    print(f"Parsed {len(result_df)} benchmarks")
    print(f"Unique data types: {result_df['data_type'].unique()}")
    print(f"Unique operations: {result_df['operation'].unique()}")
    
    return result_df

def group_benchmarks(df):
    """Group benchmarks by data_type and operation."""
    grouped = {}
    
    # Get all unique data_type and operation combinations
    data_types = df['data_type'].unique()
    operations = df['operation'].unique()
    
    for dtype in data_types:
        for op in operations:
            simd = df[(df['data_type'] == dtype) & 
                      (df['operation'] == op) & 
                      (df['category'] == 'SIMD')]
            plain = df[(df['data_type'] == dtype) & 
                       (df['operation'] == op) & 
                       (df['category'] == 'Plain')]
            
            if not simd.empty and not plain.empty:
                key = f"{dtype}_{op}"
                grouped[key] = {
                    'simd': simd.copy(),
                    'plain': plain.copy(),
                    'data_type': dtype,
                    'operation': op
                }
    
    return grouped

def calculate_speedups(grouped_benchmarks):
    """Calculate speedup ratios for each benchmark group."""
    for key, group in grouped_benchmarks.items():
        simd_df = group['simd']
        plain_df = group['plain']
        
        # Merge on size to compare matching benchmarks
        merged = pd.merge(simd_df, plain_df, on='size', suffixes=('_simd', '_plain'))
        
        # Calculate speedup (plain/simd - 1) * 100%
        merged['speedup_percent'] = (merged['time_ms_plain'] / merged['time_ms_simd'] - 1) * 100
        
        # Add back to the group
        grouped_benchmarks[key]['comparison'] = merged
    
    return grouped_benchmarks

def plot_comparisons(grouped_benchmarks, output_dir):
    """Generate a single consolidated comparison plot for all benchmark groups."""
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Get current timestamp for the generation time
    generation_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Collect data for consolidated plot
    categories = []
    simd_times = []
    plain_times = []
    speedups = []
    data_types = []
    operations = []
    
    for key, group in grouped_benchmarks.items():
        if 'comparison' in group and not group['comparison'].empty:
            comp_df = group['comparison']
            
            # Collect for consolidated plot
            for _, row in comp_df.iterrows():
                # Create a descriptive category name
                category_name = f"{row['data_type_simd']} {row['operation_simd']}"
                categories.append(category_name)
                data_types.append(row['data_type_simd'])
                operations.append(row['operation_simd'])
                simd_times.append(row['time_ms_simd'])
                plain_times.append(row['time_ms_plain'])
                speedups.append(row['speedup_percent'])
    
    # Sort the data by speedup in decreasing order
    speedup_x = [speedup / 100 + 1 for speedup in speedups]
    sorted_indices = np.argsort(speedup_x)[::-1]  # Sort in decreasing order
    categories = [categories[i] for i in sorted_indices]
    data_types = [data_types[i] for i in sorted_indices]
    operations = [operations[i] for i in sorted_indices]
    simd_times = [simd_times[i] for i in sorted_indices]
    plain_times = [plain_times[i] for i in sorted_indices]
    speedups = [speedups[i] for i in sorted_indices]
    speedup_x = [speedup_x[i] for i in sorted_indices]
    
    # Create the consolidated comparison plot
    plt.figure(figsize=(18, 10))
    bar_width = 0.35
    x = np.arange(len(categories))
   
    
    # Also create a speedup chart
    plt.figure(figsize=(18, 10))
    colors = ['limegreen' if s > 0 else 'red' for s in speedups]
    plt.bar(x, speedup_x, 0.5, color=colors)
    
    plt.xlabel('Benchmark Category', fontsize=12)
    plt.ylabel('Speedup', fontsize=12)
    compiler_text = ("GCC " + gcc_version) if os_platform == 'Linux' else (("MSVC " + msvc_version) if os_platform == 'Windows' else "Unknown Compiler") 
    plt.title(f"SIMD Speedup Over Plain Implementation ({os_platform} {compiler_text})", fontsize=14, weight='bold')
    
    # Add speedup values as text
    for i in range(len(categories)):
        va = 'bottom' if speedup_x[i] > 1 else 'top'
        offset = 0.05 if speedup_x[i] > 1 else -0.15
        plt.text(i, speedup_x[i] + offset, f"{speedup_x[i]:.2f}x", ha='center', va=va, fontsize=10, weight='bold')
    
    # Format y-axis ticks to show "x" suffix
    from matplotlib.ticker import FuncFormatter
    def format_speedup(value, pos):
        return f"{value:.0f}x"
    plt.gca().yaxis.set_major_formatter(FuncFormatter(format_speedup))
    
    plt.xticks(x, categories, rotation=45, ha='right', fontsize=10)
    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    # Get CPU info including cores, architecture and add it to the plot as a box on top right
    cpu_info = cpuinfo.get_cpu_info()
    cpu_name = cpu_info.get('brand_raw', 'N/A')
    cpu_arch = cpu_info.get('arch_string_raw', 'N/A')
    cpu_cores = cpu_info.get('count', 'N/A')
    cpu_freq_actual = cpu_info.get('hz_actual_friendly', 'N/A')
    cpu_freq_advertised = cpu_info.get('hz_advertised_friendly', 'N/A')

    info_text = (
        f"CPU: {cpu_name}\n"
        f"Arch: {cpu_arch}\n"
        f"Cores: {cpu_cores}\n"
        f"Freq (Actual): {cpu_freq_actual}\n"
        f"Freq (Advertised): {cpu_freq_advertised}"
    )

    # Position the text box on the top right
    plt.text(0.80, 0.98, info_text, transform=plt.gca().transAxes,
             fontsize=9, verticalalignment='top', horizontalalignment='left',
             bbox=dict(boxstyle='round,pad=0.5', fc='wheat', alpha=0.5))
    
    # Add generation time in a separate box below CPU info
    timestamp_text = f"Generated: {generation_time}"
    plt.text(0.5, 0.98, timestamp_text, transform=plt.gca().transAxes,
             fontsize=9, verticalalignment='top', horizontalalignment='center')
             
    plt.savefig(output_path / "consolidated_speedup.png", dpi=300)
    
    # Create a table plot with the data
    plt.figure(figsize=(20, 12))
    plt.axis('off')
    
    plt.close('all')

def generate_summary_report(grouped_benchmarks, output_dir):
    """Generate a summary report of performance comparisons."""
    output_path = Path(output_dir)
    
    with open(output_path / "summary_report.txt", "w") as f:
        f.write("# SIMD Performance Comparison Summary\n\n")
        
        for key, group in grouped_benchmarks.items():
            if 'comparison' not in group or group['comparison'].empty:
                continue
                
            comp_df = group['comparison']
            data_type = group['data_type']
            operation = group['operation']
            
            f.write(f"#### {data_type} {operation}\n\n")
            f.write("| Variant | SIMD Time (ms) | Plain Time (ms) | Speedup (x) |\n")
            f.write("|---------|---------------|----------------|------------|\n")
            
            for _, row in comp_df.iterrows():
                simd_time = row['time_ms_simd']
                plain_time = row['time_ms_plain']
                speedup = row['speedup_percent']
                
                f.write(f"| {row['size']} | {simd_time:.3f} | {plain_time:.3f} | {speedup/100.0 + 1:.2f}x |\n")
            
            f.write("\n")

def parse_unit_tests(file_path):
    """Parse unit test results from file."""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Find all test runs and their results
    run_pattern = r'\[ RUN      \] ([^\n]+)'
    run_matches = re.findall(run_pattern, content)
    
    # Find all passed tests
    passed_pattern = r'\[       OK \] ([^\n]+) \((\d+) ms\)'
    passed_matches = re.findall(passed_pattern, content)
    
    # Find all failed tests
    failed_pattern = r'\[  FAILED  \] ([^\n]+)(?: \(\d+ ms\))?'
    failed_matches = re.findall(failed_pattern, content)
    
    # Convert to dictionaries for easier lookup
    passed_dict = {test_name: int(duration) for test_name, duration in passed_matches}
    failed_dict = {test_name: -1 for test_name in failed_matches}
    
    if not run_matches:
        print("No unit test results found in the file.")
        return None
    
    results = []
    for test_name in run_matches:
        test_parts = test_name.split('.')
        test_suite = test_parts[0] if len(test_parts) > 0 else "Unknown"
        test_case = test_parts[1] if len(test_parts) > 1 else test_name
        
        if test_name in passed_dict:
            # Test passed
            duration_ms = passed_dict[test_name]
            status = "PASSED"
        elif test_name in failed_dict:
            # Test failed
            duration_ms = -1
            status = "FAILED"
        else:
            # Unknown status (shouldn't happen if the test output is well-formed)
            duration_ms = -1
            status = "UNKNOWN"
        
        results.append({
            'test_suite': test_suite,
            'test_case': test_case,
            'duration_ms': duration_ms,
            'status': status
        })
    
    # Create a DataFrame
    result_df = pd.DataFrame(results)
    
    if not result_df.empty:
        print(f"Parsed {len(result_df)} unit tests")
        print(f"Passed: {(result_df['status'] == 'PASSED').sum()}, Failed: {(result_df['status'] == 'FAILED').sum()}")
    
    return result_df

def plot_unit_tests(unit_tests_df, output_dir):
    """Generate a table visualization of unit test results."""
    if unit_tests_df is None or unit_tests_df.empty:
        print("No unit test data to plot.")
        return
    
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Get current timestamp for the generation time
    generation_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Create a figure for the unit tests table
    plt.figure(figsize=(12, max(6, len(unit_tests_df) * 0.4)))
    plt.axis('off')
    
    # Prepare data for the table
    cell_text = []
    cell_colors = []
    
    # Sort by test suite and test case
    unit_tests_df = unit_tests_df.sort_values(['test_suite', 'test_case'])
    
    for _, row in unit_tests_df.iterrows():
        status_text = "✓ Pass" if row['status'] == "PASSED" else "X Fail"
        duration_text = f"{row['duration_ms']} ms" if row['duration_ms'] >= 0 else "N/A"
        
        cell_text.append([row['test_suite'], row['test_case'], status_text, duration_text])
        
        # Choose row color based on test status
        if row['status'] == "PASSED":
            cell_colors.append(['lightgreen', 'lightgreen', 'lightgreen', 'lightgreen'])  # Light green for passed
        else:
            cell_colors.append(['lightred', 'lightred', 'lightred', 'lightred'])  # Light red for failed
    
    # Create the table
    table = plt.table(
        cellText=cell_text,
        colLabels=['Test Suite', 'Test Case', 'Status', 'Duration'],
        loc='center',
        cellLoc='center',
        cellColours=cell_colors,
        colWidths=[0.2, 0.4, 0.2, 0.2]
    )
    
    # Styling the table
    table.auto_set_font_size(False)
    table.set_fontsize(12)
    table.scale(1, 1.5)  # Adjust row heights
    
    # Add a title
    compiler_text = ("GCC " + gcc_version) if os_platform == 'Linux' else (("MSVC " + msvc_version) if os_platform == 'Windows' else "Unknown Compiler") 
    plt.title(f'Unit Tests Results ({os_platform} {compiler_text})', fontsize=16, pad=20)
    
    plt.figtext(0.5, 0.98, f"Generated: {generation_time}", transform=plt.gca().transAxes,
             fontsize=9, verticalalignment='top', horizontalalignment='center')
    
    # Get summary statistics
    total_tests = len(unit_tests_df)
    passed_tests = unit_tests_df[unit_tests_df['status'] == "PASSED"].shape[0]
    failed_tests = total_tests - passed_tests
    
    # Add summary text at the top
    summary_text = f"Total: {total_tests} | Passed: {passed_tests} | Failed: {failed_tests}"
    plt.figtext(0.5, 0.01, summary_text, 
                ha='center', fontsize=12, weight='bold')
    
    plt.tight_layout()
    plt.savefig(output_path / "unit_test_results.png", dpi=300, bbox_inches='tight')
    plt.close()

def main():
    global gcc_version
    global msvc_version
    global os_platform
    
    os_platform = platform.system()
    
    if os_platform == 'Linux':
        output = subprocess.check_output(['gcc', '--version'], stderr=subprocess.STDOUT)
        output = output.decode('utf-8')
        gcc_version = re.search(r'(\d+\.\d+\.\d+)', output).group(1)
        print(f"GCC version: {gcc_version}")
    elif os_platform == 'Windows':
        try:
            result = subprocess.run(
                [
                    r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
                    "-latest",
                    "-products", "*",
                    "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                    "-property", "catalog_productDisplayVersion"
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True
            )
            msvc_version = result.stdout.strip()
        except subprocess.CalledProcessError as e:
            print(f"Error getting MSVC version: {e}")
            return None
    
    """Main function to run the benchmark analysis."""
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Analyze SIMD benchmark results.')
    parser.add_argument('--input_file', '-i',
                        required=True,
                        help='Path to the benchmark results file')
    parser.add_argument('--output_dir', '-o',
                        required=True,
                        help='Directory to save analysis results')
    args = parser.parse_args()
    
    input_file = args.input_file
    output_dir = args.output_dir
    
    print(f"Analyzing benchmarks from: {input_file}")
    print(f"Saving results to: {output_dir}")
    
    # Parse unit test results
    unit_tests_df = parse_unit_tests(input_file)
    # Plot unit test results
    if unit_tests_df is not None and not unit_tests_df.empty:
        plot_unit_tests(unit_tests_df, output_dir)
    
    # Parse benchmark results
    df = parse_benchmark_results(input_file)
    
    if df is None or df.empty:
        print("No benchmark data found.")
        return
    # Group benchmarks
    grouped = group_benchmarks(df)
    
    # Calculate speedups
    grouped = calculate_speedups(grouped)
    
    # Plot comparisons
    plot_comparisons(grouped, output_dir)
    
    
    
    # Generate summary report
    generate_summary_report(grouped, output_dir)
    
    print(f"Analysis complete. Results saved to {output_dir}")

if __name__ == "__main__":
    main()