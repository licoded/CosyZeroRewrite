# [175] chore: add results/ to .gitignore and remove from tracking

**Commit**: `2d49c2d` ([`2d49c2dd59a495069ef2aa81d0694a5ca5eaabe0`](https://github.com/licoded/CosyZeroRewrite/commit/2d49c2dd59a495069ef2aa81d0694a5ca5eaabe0))
**Date**: 2026-01-04 10:48:50 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add results/ directory to .gitignore to prevent test outputs
from being committed. This includes:
- Benchmark results CSV files
- Game graph DOT/JSON exports
- Other test outputs

Also remove previously tracked CSV files from git index.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
将 results/ 目录加入 .gitignore，避免测试输出文件被提交。同时从 git 索引中清除之前误跟踪的 CSV 文件。

### 🔍 Technical Details
- results/ 目录包含：benchmark CSV、游戏图 DOT/JSON、其他测试输出
- 使用 `git rm -r --cached results/` 从索引中删除但保留本地文件
- 之后 results/ 下的所有新生成文件都不会被 git 跟踪

### 📊 Impact Analysis
- **仓库大小**: 减少 git 仓库体积（不再存储测试输出）
- **开发体验**: 避免误提交测试结果，保持仓库干净

## Changes

### Modified
- `.gitignore`


### Deleted
- `results/benchmark/serial/2026-01-03/serial_benchmark_20260103_231103.csv`
- `results/benchmark/serial/2026-01-03/serial_benchmark_20260103_231148.csv`


## Stats

- **3** files changed
- **3** insertions(+)
- **10** deletions(-)
