# [81] feat: save benchmark CSV results to organized results/ directory

**Commit**: `c17b31b` ([`c17b31b5bd93f50644562297ce8c3713069caa4e`](https://github.com/licoded/CosyZeroRewrite/commit/c17b31b5bd93f50644562297ce8c3713069caa4e))
**Date**: 2026-01-02 14:02:21 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Create results/benchmark/YYYY-MM-DD/HH-MM/ for CSV files
- Keep logs/benchmark/YYYY-MM-DD/HH-MM/ for log files
- Also save benchmark_results_latest.csv in build for convenience
- Results and logs are now in separate directories

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
结果目录分离：创建 `results/benchmark/` 目录单独存放 CSV 结果，与 `logs/benchmark/` 日志目录分开。

### 🔍 Technical Details

**目录结构**：
```
logs/benchmark/YYYY-MM-DD/HH-MM/    # 日志文件
results/benchmark/YYYY-MM-DD/HH-MM/ # CSV 结果
build/benchmark_results_latest.csv   # 便捷访问
```

**好处**：
- 日志和结果文件分开管理
- 按时间戳组织便于历史追踪
- build 目录保留最新结果便于快速访问

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **31** insertions(+)
- **4** deletions(-)
