#!/usr/bin/env python3
"""
Visualize benchmark results from benchmark_runner.csv

Usage:
    python visualize_results.py benchmark_results_1234567890.csv
"""

import sys
import csv
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def load_results(csv_path):
    """Load benchmark results from CSV file."""
    results = []
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            results.append(row)
    return results

def analyze_results(results):
    """Analyze results and compute statistics."""
    total = len(results)
    passed = sum(1 for r in results if r['Match'] == 'PASS')
    failed = sum(1 for r in results if r['Match'] == 'FAIL')
    errors = sum(1 for r in results if r['Status'] == 'ERROR')

    true_positives = sum(1 for r in results
                         if r['Match'] == 'PASS' and r['Computed'] == 'Realizable')
    true_negatives = sum(1 for r in results
                         if r['Match'] == 'PASS' and r['Computed'] == 'Unrealizable')
    false_positives = sum(1 for r in results
                          if r['Match'] == 'FAIL' and r['Computed'] == 'Realizable')
    false_negatives = sum(1 for r in results
                          if r['Match'] == 'FAIL' and r['Computed'] == 'Unrealizable')

    times = [float(r['TimeMs']) for r in results if r['Status'] == 'SUCCESS']

    return {
        'total': total,
        'passed': passed,
        'failed': failed,
        'errors': errors,
        'true_positives': true_positives,
        'true_negatives': true_negatives,
        'false_positives': false_positives,
        'false_negatives': false_negatives,
        'accuracy': 100 * passed / total if total > 0 else 0,
        'avg_time': np.mean(times) if times else 0,
        'median_time': np.median(times) if times else 0,
        'max_time': max(times) if times else 0,
        'min_time': min(times) if times else 0,
    }

def plot_confusion_matrix(stats, output_dir):
    """Create confusion matrix visualization."""
    fig, ax = plt.subplots(figsize=(8, 6))

    tp = stats['true_positives']
    tn = stats['true_negatives']
    fp = stats['false_positives']
    fn = stats['false_negatives']

    # Create confusion matrix
    # Rows: Actual (Realizable, Unrealizable)
    # Cols: Predicted (Realizable, Unrealizable)
    cm = np.array([[tp, fn], [fp, tn]])

    im = ax.imshow(cm, cmap='Blues', alpha=0.7)
    ax.set_xticks(np.arange(2))
    ax.set_yticks(np.arange(2))
    ax.set_xticklabels(['Predicted Realizable', 'Predicted Unrealizable'])
    ax.set_yticklabels(['Actual Realizable', 'Actual Unrealizable'])

    # Add text annotations
    for i in range(2):
        for j in range(2):
            text = ax.text(j, i, cm[i, j],
                         ha='center', va='center', color='black', fontsize=14)

    ax.set_title('Confusion Matrix', fontsize=14, fontweight='bold')
    plt.tight_layout()
    plt.savefig(output_dir / 'confusion_matrix.png', dpi=150)
    plt.close()
    print(f"Saved: {output_dir / 'confusion_matrix.png'}")

def plot_time_distribution(results, output_dir):
    """Plot time distribution."""
    times = [float(r['TimeMs']) for r in results if r['Status'] == 'SUCCESS']
    statuses = ['PASS' if r['Match'] == 'PASS' else 'FAIL' for r in results if r['Status'] == 'SUCCESS']

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    # Histogram
    ax1.hist(times, bins=30, color='steelblue', alpha=0.7, edgecolor='black')
    ax1.set_xlabel('Time (ms)', fontsize=12)
    ax1.set_ylabel('Count', fontsize=12)
    ax1.set_title('Distribution of Synthesis Time', fontsize=14, fontweight='bold')
    ax1.axvline(np.median(times), color='red', linestyle='--', label=f'Median: {np.median(times):.2f}ms')
    ax1.axvline(np.mean(times), color='green', linestyle='--', label=f'Mean: {np.mean(times):.2f}ms')
    ax1.legend()

    # Box plot by status
    pass_times = [float(r['TimeMs']) for r in results if r['Status'] == 'SUCCESS' and r['Match'] == 'PASS']
    fail_times = [float(r['TimeMs']) for r in results if r['Status'] == 'SUCCESS' and r['Match'] == 'FAIL']

    bp = ax2.boxplot([pass_times, fail_times], labels=['Pass', 'Fail'], patch_artist=True)
    for patch, color in zip(bp['boxes'], ['lightgreen', 'lightcoral']):
        patch.set_facecolor(color)
    ax2.set_ylabel('Time (ms)', fontsize=12)
    ax2.set_title('Time by Result Status', fontsize=14, fontweight='bold')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_dir / 'time_distribution.png', dpi=150)
    plt.close()
    print(f"Saved: {output_dir / 'time_distribution.png'}")

def plot_summary(stats, output_dir):
    """Create summary statistics visualization."""
    fig, ax = plt.subplots(figsize=(10, 6))

    categories = ['Passed\n(Correct)', 'Failed\n(Mismatch)', 'Errors']
    values = [stats['passed'], stats['failed'], stats['errors']]
    colors = ['lightgreen', 'lightcoral', 'gray']

    bars = ax.bar(categories, values, color=colors, edgecolor='black', alpha=0.8)

    # Add value labels on bars
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}',
                ha='center', va='bottom', fontsize=12, fontweight='bold')

    # Add accuracy text
    ax.text(0.5, max(values) * 0.9, f"Accuracy: {stats['accuracy']:.1f}%",
            ha='center', va='center', fontsize=16, fontweight='bold',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    ax.set_ylabel('Count', fontsize=12)
    ax.set_title(f'Benchmark Results Summary (Total: {stats["total"]})',
                 fontsize=14, fontweight='bold')
    ax.set_ylim(0, max(values) * 1.1)

    plt.tight_layout()
    plt.savefig(output_dir / 'summary.png', dpi=150)
    plt.close()
    print(f"Saved: {output_dir / 'summary.png'}")

def plot_per_folder_results(results, output_dir):
    """Plot results per benchmark folder."""
    # Group by folder
    folder_stats = {}
    for r in results:
        folder = r['Folder']
        if folder not in folder_stats:
            folder_stats[folder] = {'total': 0, 'pass': 0, 'fail': 0, 'error': 0}
        folder_stats[folder]['total'] += 1
        if r['Status'] == 'ERROR':
            folder_stats[folder]['error'] += 1
        elif r['Match'] == 'PASS':
            folder_stats[folder]['pass'] += 1
        else:
            folder_stats[folder]['fail'] += 1

    # Sort folders
    sorted_folders = sorted(folder_stats.keys())

    fig, ax = plt.subplots(figsize=(12, 6))

    folders = sorted_folders
    passes = [folder_stats[f]['pass'] for f in folders]
    fails = [folder_stats[f]['fail'] for f in folders]
    errors = [folder_stats[f]['error'] for f in folders]

    x = np.arange(len(folders))
    width = 0.6

    ax.bar(x, passes, width, label='Pass', color='lightgreen', edgecolor='black')
    ax.bar(x, fails, width, bottom=passes, label='Fail', color='lightcoral', edgecolor='black')
    ax.bar(x, errors, width, bottom=[p+f for p, f in zip(passes, fails)],
           label='Error', color='gray', edgecolor='black')

    ax.set_ylabel('Count', fontsize=12)
    ax.set_title('Results by Benchmark Folder', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(folders, rotation=45, ha='right')
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig(output_dir / 'per_folder.png', dpi=150)
    plt.close()
    print(f"Saved: {output_dir / 'per_folder.png'}")

def generate_html_report(results, stats, output_dir):
    """Generate HTML report with all visualizations."""
    html_path = output_dir / 'report.html'

    html = f"""<!DOCTYPE html>
<html>
<head>
    <title>Benchmark Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        h1 {{ color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }}
        h2 {{ color: #666; margin-top: 30px; }}
        .stats {{ display: grid; grid-template-columns: repeat(4, 1fr); gap: 15px; margin: 20px 0; }}
        .stat-box {{ background: #f9f9f9; padding: 15px; border-radius: 8px; text-align: center; border-left: 4px solid #4CAF50; }}
        .stat-box.failed {{ border-left-color: #f44336; }}
        .stat-box.error {{ border-left-color: #9e9e9e; }}
        .stat-value {{ font-size: 28px; font-weight: bold; color: #333; }}
        .stat-label {{ font-size: 14px; color: #666; margin-top: 5px; }}
        .chart {{ text-align: center; margin: 30px 0; }}
        .chart img {{ max-width: 100%; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }}
        table {{ width: 100%; border-collapse: collapse; margin-top: 20px; }}
        th, td {{ padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }}
        th {{ background: #4CAF50; color: white; }}
        tr:hover {{ background: #f5f5f5; }}
        .pass {{ color: green; font-weight: bold; }}
        .fail {{ color: red; font-weight: bold; }}
        .error {{ color: gray; font-weight: bold; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🧪 LTLf Synthesis Benchmark Report</h1>

        <div class="stats">
            <div class="stat-box">
                <div class="stat-value">{stats['total']}</div>
                <div class="stat-label">Total Tests</div>
            </div>
            <div class="stat-box">
                <div class="stat-value">{stats['accuracy']:.1f}%</div>
                <div class="stat-label">Accuracy</div>
            </div>
            <div class="stat-box failed">
                <div class="stat-value">{stats['failed']}</div>
                <div class="stat-label">Failed</div>
            </div>
            <div class="stat-box">
                <div class="stat-value">{stats['avg_time']:.2f}ms</div>
                <div class="stat-label">Avg Time</div>
            </div>
        </div>

        <h2>📊 Visualizations</h2>

        <div class="chart">
            <h3>Summary</h3>
            <img src="summary.png" alt="Summary">
        </div>

        <div class="chart">
            <h3>Confusion Matrix</h3>
            <img src="confusion_matrix.png" alt="Confusion Matrix">
        </div>

        <div class="chart">
            <h3>Time Distribution</h3>
            <img src="time_distribution.png" alt="Time Distribution">
        </div>

        <div class="chart">
            <h3>Results by Folder</h3>
            <img src="per_folder.png" alt="Per Folder Results">
        </div>

        <h2>📋 Detailed Results</h2>
        <table>
            <tr>
                <th>Folder</th>
                <th>Filename</th>
                <th>Expected</th>
                <th>Computed</th>
                <th>Result</th>
                <th>Time (ms)</th>
            </tr>
"""

    for r in results[:100]:  # Limit to first 100 for HTML
        status_class = 'pass' if r['Match'] == 'PASS' else 'fail' if r['Status'] == 'SUCCESS' else 'error'
        status_text = r['Match'] if r['Status'] == 'SUCCESS' else r['Status']

        html += f"""
            <tr>
                <td>{r['Folder']}</td>
                <td>{r['Filename']}</td>
                <td>{r['Expected']}</td>
                <td>{r['Computed']}</td>
                <td class="{status_class}">{status_text}</td>
                <td>{r['TimeMs']}</td>
            </tr>
"""

    html += """
        </table>
    </div>
</body>
</html>
"""

    with open(html_path, 'w') as f:
        f.write(html)

    print(f"Saved: {html_path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python visualize_results.py <benchmark_results.csv>")
        sys.exit(1)

    csv_path = Path(sys.argv[1])
    if not csv_path.exists():
        print(f"Error: File not found: {csv_path}")
        sys.exit(1)

    # Create output directory
    output_dir = csv_path.parent / 'benchmark_visualization'
    output_dir.mkdir(exist_ok=True)

    print(f"Loading results from: {csv_path}")
    results = load_results(csv_path)
    stats = analyze_results(results)

    print("\n=== Benchmark Statistics ===")
    print(f"Total:       {stats['total']}")
    print(f"Passed:      {stats['passed']} ({100*stats['passed']/stats['total']:.1f}%)")
    print(f"Failed:      {stats['failed']} ({100*stats['failed']/stats['total']:.1f}%)")
    print(f"Errors:      {stats['errors']} ({100*stats['errors']/stats['total']:.1f}%)")
    print(f"Accuracy:    {stats['accuracy']:.2f}%")
    print(f"\nConfusion Matrix:")
    print(f"  True Positives:  {stats['true_positives']} (correctly Realizable)")
    print(f"  True Negatives:  {stats['true_negatives']} (correctly Unrealizable)")
    print(f"  False Positives: {stats['false_positives']} (incorrectly Realizable)")
    print(f"  False Negatives: {stats['false_negatives']} (incorrectly Unrealizable)")
    print(f"\nTime Statistics:")
    print(f"  Average:  {stats['avg_time']:.2f} ms")
    print(f"  Median:   {stats['median_time']:.2f} ms")
    print(f"  Min:      {stats['min_time']:.2f} ms")
    print(f"  Max:      {stats['max_time']:.2f} ms")

    print(f"\nGenerating visualizations in: {output_dir}")
    plot_summary(stats, output_dir)
    plot_confusion_matrix(stats, output_dir)
    plot_time_distribution(results, output_dir)
    plot_per_folder_results(results, output_dir)
    generate_html_report(results, stats, output_dir)

    print(f"\n✅ Complete! Open {output_dir / 'report.html'} in a browser to view the report.")

if __name__ == '__main__':
    main()
