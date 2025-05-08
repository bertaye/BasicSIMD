import re
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path
import argparse  # Add argparse for command line arguments

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
    
    # Sort the data by data type and operation for better visualization
    sorted_indices = np.argsort([f"{dt}_{op}" for dt, op in zip(data_types, operations)])
    categories = [categories[i] for i in sorted_indices]
    data_types = [data_types[i] for i in sorted_indices]
    operations = [operations[i] for i in sorted_indices]
    simd_times = [simd_times[i] for i in sorted_indices]
    plain_times = [plain_times[i] for i in sorted_indices]
    speedups = [speedups[i] for i in sorted_indices]
    
    # Create the consolidated comparison plot
    plt.figure(figsize=(18, 10))
    bar_width = 0.35
    x = np.arange(len(categories))
    
    # Create a bar chart with SIMD and Plain implementations
    plt.bar(x - bar_width/2, simd_times, bar_width, label='SIMD', color='royalblue')
    plt.bar(x + bar_width/2, plain_times, bar_width, label='Plain', color='lightcoral')
    
    plt.xlabel('Benchmark Category', fontsize=12)
    plt.ylabel('Time (ms)', fontsize=12)
    plt.title('SIMD vs Plain Performance Comparison', fontsize=14)
    
    # Add speedup text on top of bars
    for i in range(len(categories)):
        speedup = speedups[i]
        color = 'green' if speedup > 0 else 'red'
        position = max(simd_times[i], plain_times[i]) + 0.002
        plt.text(i, position, f"{speedup:.1f}%", ha='center', color=color, weight='bold')
    
    plt.xticks(x, categories, rotation=45, ha='right', fontsize=10)
    plt.legend(fontsize=12)
    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.savefig(output_path / "consolidated_comparison.png", dpi=300)
    
    # Also create a speedup chart
    plt.figure(figsize=(18, 10))
    colors = ['green' if s > 0 else 'red' for s in speedups]
    plt.bar(x, speedups, color=colors)
    plt.axhline(y=0, color='k', linestyle='-', alpha=0.3)
    
    plt.xlabel('Benchmark Category', fontsize=12)
    plt.ylabel('Speedup (%)', fontsize=12)
    plt.title('SIMD Speedup over Plain Implementation', fontsize=14)
    
    # Add speedup values as text
    for i in range(len(categories)):
        va = 'bottom' if speedups[i] > 0 else 'top'
        offset = 2 if speedups[i] > 0 else -2
        plt.text(i, speedups[i] + offset, f"{speedups[i]:.1f}%", ha='center', va=va, fontsize=10)
    
    plt.xticks(x, categories, rotation=45, ha='right', fontsize=10)
    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.savefig(output_path / "consolidated_speedup.png", dpi=300)
    
    # Create a table plot with the data
    plt.figure(figsize=(20, 12))
    plt.axis('off')
    
    # Create table data
    table_data = []
    for i in range(len(categories)):
        table_data.append([
            categories[i], 
            f"{simd_times[i]:.3f}", 
            f"{plain_times[i]:.3f}", 
            f"{speedups[i]:+.1f}%"
        ])
    
    # Sort by data type and operation for better readability
    table_data.sort(key=lambda x: x[0])
    
    # Create the table
    table = plt.table(
        cellText=table_data,
        colLabels=['Benchmark', 'SIMD (ms)', 'Plain (ms)', 'Speedup (%)'],
        loc='center',
        cellLoc='center',
        colWidths=[0.4, 0.2, 0.2, 0.2]
    )
    
    # Styling the table
    table.auto_set_font_size(False)
    table.set_fontsize(12)
    table.scale(1, 1.5)  # Adjust row heights
    
    # Color the speedup cells
    for i in range(len(table_data)):
        speedup = float(table_data[i][3].strip('%+'))
        if speedup > 0:
            table[(i+1, 3)].set_facecolor('#d5f4e6')  # Light green
        else:
            table[(i+1, 3)].set_facecolor('#f4d5d5')  # Light red
    
    plt.title('SIMD vs Plain Performance Comparison Table', fontsize=16, pad=20)
    plt.tight_layout()
    plt.savefig(output_path / "comparison_table.png", dpi=300, bbox_inches='tight')
    
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
            
            f.write(f"## {data_type} {operation}\n\n")
            f.write("| Variant | SIMD Time (ms) | Plain Time (ms) | Speedup (%) |\n")
            f.write("|---------|---------------|----------------|------------|\n")
            
            for _, row in comp_df.iterrows():
                simd_time = row['time_ms_simd']
                plain_time = row['time_ms_plain']
                speedup = row['speedup_percent']
                
                f.write(f"| {row['size']} | {simd_time:.3f} | {plain_time:.3f} | {speedup:.2f} |\n")
            
            f.write("\n")

def main():
    """Main function to run the benchmark analysis."""
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Analyze SIMD benchmark results.')
    parser.add_argument('--input', '-i',
                        required=True,
                        help='Path to the benchmark results file')
    parser.add_argument('--output', '-o',
                        required=True,
                        help='Directory to save analysis results')
    args = parser.parse_args()
    
    input_file = args.input
    output_dir = args.output
    
    print(f"Analyzing benchmarks from: {input_file}")
    print(f"Saving results to: {output_dir}")
    
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