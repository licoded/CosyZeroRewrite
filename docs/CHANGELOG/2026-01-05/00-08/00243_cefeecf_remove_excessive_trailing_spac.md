# [243] fix: remove excessive trailing spaces in progress bar

**Commit**: `cefeecf` ([`cefeecfbc89b8857382009a09eba96c362a0a765`](https://github.com/licoded/CosyZeroRewrite/commit/cefeecfbc89b8857382009a09eba96c362a0a765))
**Date**: 2026-01-05 00:41:14 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Track the previous output length and only add spaces needed to clear
the previous longer output, instead of always padding to a fixed
maximum width. This makes the progress bar display cleaner.

Changes:
- Add last_output_len_ member to track previous output length
- Only pad with spaces when current output is shorter than previous
- Remove the bar_width + 80 fixed padding

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
修复进度条过度填充空白字符的问题。通过跟踪上次输出的长度，只在当前输出比上次短时才添加必要的空格来清除残留字符，避免进度条后有过多的尾随空格。

### 🔍 Technical Details
- 新增 `last_output_len_` 成员变量跟踪上次输出长度
- 只在 `current_len < last_output_len_` 时才填充空格
- 移除了之前的 `bar_width + 80` 固定填充逻辑

### 📊 Impact Analysis
- 影响组件：`ProgressDisplay` 类
- 视觉改进：进度条显示更简洁，没有多余的尾随空格
- 功能保持：终端更新行为不变，只是输出更干净

## Changes

### Modified
- `tests/bench/benchmark.cpp`


## Stats

- **1** files changed
- **10** insertions(+)
- **7** deletions(-)
