#!/usr/bin/env python3
"""
Reorganize all logs and results directory structure.
"""

import os
import re
import shutil
from datetime import datetime

def get_period(hour):
    """Get period name based on hour."""
    if 6 <= hour < 12:
        return "01-morning"
    elif 12 <= hour < 18:
        return "02-afternoon"
    elif 18 <= hour < 24:
        return "03-evening"
    else:
        return "04-night"

def parse_timestamp(filename):
    """Parse timestamp from filename."""
    # Try format: prefix_YYYYMMDD_HHMMSS.log
    match = re.search(r'(\d{8})_(\d{6})', filename)
    if match:
        date_str = match.group(1)  # YYYYMMDD
        time_str = match.group(2)  # HHMMSS
        dt = datetime.strptime(f"{date_str}{time_str}", "%Y%m%d%H%M%S")
        return dt
    return None

def reorganize_formula_logs(base_dir):
    """Reorganize formula_*.log files."""
    print("Reorganizing formula logs...")
    
    groups = {}
    pattern = re.compile(r'formula_(\d{8})_(\d{6})(?:\.\d+)?\.log')
    
    for filename in list(os.listdir(base_dir)):
        if not filename.startswith('formula_'):
            continue
            
        match = pattern.search(filename)
        if not match:
            continue
        
        dt = parse_timestamp(filename)
        if not dt:
            continue
        
        date_dir = dt.strftime("%Y-%m-%d")
        period = get_period(dt.hour)
        key = (date_dir, period)
        
        if key not in groups:
            groups[key] = []
        groups[key].append((dt, filename))
    
    for (date_dir, period), files in groups.items():
        files.sort(reverse=True, key=lambda x: x[0])
        newest_file = files[0][1]
        
        new_dir = os.path.join(base_dir, "formula", date_dir, period)
        os.makedirs(new_dir, exist_ok=True)
        new_file = os.path.join(new_dir, "formula.log")
        
        old_file = os.path.join(base_dir, newest_file)
        shutil.move(old_file, new_file)
        print(f"  formula/{newest_file} -> {date_dir}/{period}/formula.log")
        
        for _, old_filename in files[1:]:
            os.remove(os.path.join(base_dir, old_filename))

def reorganize_failures_logs(base_dir):
    """Reorganize failures_*.log files."""
    print("Reorganizing failures logs...")
    
    groups = {}
    pattern = re.compile(r'failures_(\d{8})_(\d{6})\.log')
    
    for filename in list(os.listdir(base_dir)):
        if not filename.startswith('failures_'):
            continue
        
        match = pattern.search(filename)
        if not match:
            continue
        
        dt = parse_timestamp(filename)
        if not dt:
            continue
        
        date_dir = dt.strftime("%Y-%m-%d")
        period = get_period(dt.hour)
        key = (date_dir, period)
        
        if key not in groups:
            groups[key] = []
        groups[key].append((dt, filename))
    
    for (date_dir, period), files in groups.items():
        files.sort(reverse=True, key=lambda x: x[0])
        newest_file = files[0][1]
        
        new_dir = os.path.join(base_dir, "failures", date_dir, period)
        os.makedirs(new_dir, exist_ok=True)
        new_file = os.path.join(new_dir, "failures.log")
        
        shutil.move(os.path.join(base_dir, newest_file), new_file)
        print(f"  failures/{newest_file} -> {date_dir}/{period}/failures.log")
        
        for _, old_filename in files[1:]:
            os.remove(os.path.join(base_dir, old_filename))

def reorganize_transform_logs(base_dir):
    """Reorganize transform_*.log files."""
    print("Reorganizing transform logs...")
    
    # Group by type (failures/summary)
    for log_type in ["transform_failures", "transform_summary"]:
        groups = {}
        pattern = re.compile(f'{log_type}_(\\d{{8}})_(\\d{{6}})\\.log')
        
        for filename in list(os.listdir(base_dir)):
            if not filename.startswith(log_type):
                continue
            
            match = pattern.search(filename)
            if not match:
                continue
            
            dt = parse_timestamp(filename)
            if not dt:
                continue
            
            date_dir = dt.strftime("%Y-%m-%d")
            period = get_period(dt.hour)
            key = (date_dir, period)
            
            if key not in groups:
                groups[key] = []
            groups[key].append((dt, filename))
        
        for (date_dir, period), files in groups.items():
            files.sort(reverse=True, key=lambda x: x[0])
            newest_file = files[0][1]
            
            new_dir = os.path.join(base_dir, "transform", date_dir, period)
            os.makedirs(new_dir, exist_ok=True)
            
            basename = log_type.replace("transform_", "") + ".log"
            new_file = os.path.join(new_dir, basename)
            
            shutil.move(os.path.join(base_dir, newest_file), new_file)
            print(f"  {log_type}/{newest_file} -> {date_dir}/{period}/{basename}")
            
            for _, old_filename in files[1:]:
                os.remove(os.path.join(base_dir, old_filename))

def cleanup_old_formula_logs(base_dir):
    """Clean up old incremental formula_*.log files in period directories."""
    print("Cleaning up old incremental formula logs...")

    formula_base = os.path.join(base_dir, "logs", "formula")
    if not os.path.exists(formula_base):
        print("  No formula logs directory found")
        return

    for date_dir in os.listdir(formula_base):
        date_path = os.path.join(formula_base, date_dir)
        if not os.path.isdir(date_path):
            continue

        if not re.match(r'20\d{2}-\d{2}-\d{2}', date_dir):
            continue

        for period_dir in os.listdir(date_path):
            period_path = os.path.join(date_path, period_dir)
            if not os.path.isdir(period_path):
                continue

            # Check for old incremental files: formula_<numbers>.log or formula_<numbers>_<numbers>.log etc.
            old_files = []
            for filename in os.listdir(period_path):
                if filename == "formula.log":
                    continue
                if filename.startswith("formula_") and filename.endswith(".log"):
                    old_files.append(filename)

            if not old_files:
                continue

            # Find the largest file (most complete log)
            largest_file = None
            largest_size = 0
            for f in old_files:
                filepath = os.path.join(period_path, f)
                size = os.path.getsize(filepath)
                if size > largest_size:
                    largest_size = size
                    largest_file = f

            if largest_file:
                # If formula.log doesn't exist, create it from the largest file
                target_log = os.path.join(period_path, "formula.log")
                if not os.path.exists(target_log):
                    shutil.move(os.path.join(period_path, largest_file), target_log)
                    print(f"  {date_dir}/{period_dir}/formula.log <- {largest_file}")
                    old_files.remove(largest_file)

                # Delete all other old files
                for f in old_files:
                    os.remove(os.path.join(period_path, f))
                    print(f"  deleted {date_dir}/{period_dir}/{f}")

def reorganize_benchmark_results(base_dir):
    """Reorganize results/benchmark from HH-MM to period structure."""
    print("Reorganizing benchmark results...")
    
    benchmark_dir = os.path.join(base_dir, "results", "benchmark")
    if not os.path.exists(benchmark_dir):
        print("  No benchmark results directory found")
        return
    
    for date_dir in os.listdir(benchmark_dir):
        date_path = os.path.join(benchmark_dir, date_dir)
        if not os.path.isdir(date_path):
            continue
        
        if not re.match(r'20\d{2}-\d{2}-\d{2}', date_dir):
            continue
        
        # Check if already using new structure
        subdirs = [d for d in os.listdir(date_path) if os.path.isdir(os.path.join(date_path, d))]
        has_new = any(d.startswith(("01-", "02-", "03-", "04-")) for d in subdirs)
        has_old = any(re.match(r'\d{2}-\d{2}', d) for d in subdirs)
        
        if not has_old:
            continue
        
        # Collect files by period
        period_files = {}
        for time_dir in subdirs:
            if not re.match(r'\d{2}-\d{2}', time_dir):
                continue
            
            hour = int(time_dir.split('-')[0])
            period = get_period(hour)
            time_path = os.path.join(date_path, time_dir)
            
            for csv_file in os.listdir(time_path):
                if csv_file.endswith('.csv'):
                    src_file = os.path.join(time_path, csv_file)
                    if period not in period_files:
                        period_files[period] = []
                    period_files[period].append((src_file, csv_file))
        
        # Move files to period dirs
        for period, files in period_files.items():
            new_dir = os.path.join(date_path, period)
            os.makedirs(new_dir, exist_ok=True)
            
            for src_file, csv_file in files:
                new_file = os.path.join(new_dir, csv_file)
                shutil.move(src_file, new_file)
                print(f"  results/benchmark/{date_dir}/{time_dir}/{csv_file} -> {period}/{csv_file}")
        
        # Remove empty old directories
        for time_dir in subdirs:
            if re.match(r'\d{2}-\d{2}', time_dir):
                try:
                    os.rmdir(os.path.join(date_path, time_dir))
                except:
                    pass

def main():
    project_dir = "/home/lic/files/rewrite_ltlf_codes/CosyZeroRewrite"
    logs_dir = os.path.join(project_dir, "logs")
    
    print("=" * 60)
    print("Reorganizing logs and results")
    print("=" * 60)
    
    reorganize_formula_logs(logs_dir)
    reorganize_failures_logs(logs_dir)
    reorganize_transform_logs(logs_dir)
    cleanup_old_formula_logs(project_dir)
    reorganize_benchmark_results(project_dir)
    
    print("=" * 60)
    print("Done!")
    print("=" * 60)

if __name__ == "__main__":
    main()
