# [77] feat: implement progressive timeout strategy for benchmark runner

**Commit**: `1f97795` ([`1f977958e72383361a2672b5ea65bf7fa1be491b`](https://github.com/licoded/CosyZeroRewrite/commit/1f977958e72383361a2672b5ea65bf7fa1be491b))
**Date**: 2026-01-02 13:16:13 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Improves benchmark runner with multi-stage timeout approach:
- Stage 1: 1 minute timeout per formula
- Stage 2: 3 minute timeout for previous timeouts
- Stage 3: 5 minute timeout for remaining timeouts
- Progress report after each stage

Technical changes:
- Use std::async with wait_for() for timeout control
- Track which formulas timed out at each stage
- Generate detailed summary with confusion matrix
- Output includes timeout tracking and expanded states count

This allows fast formulas to complete quickly while giving
complex formulas more time as needed.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
渐进式超时策略：实现多阶段超时方法 (1分钟 → 3分钟 → 5分钟)，平衡快速公式完成时间和复杂公式需求。

### 🔍 Technical Details

**三阶段超时**：
- 阶段 1: 1 分钟超时 (快速公式)
- 阶段 2: 3 分钟超时 (之前超时的公式)
- 阶段 3: 5 分钟超时 (剩余超时公式)

**技术实现**：
- 使用 `std::async` + `wait_for()` 控制超时
- 跟踪每个阶段超时的公式
- 生成详细摘要 (含混淆矩阵)
- 输出超时跟踪和扩展状态计数

## Changes

### Modified
- `tools/benchmark_runner.cpp`


## Stats

- **1** files changed
- **168** insertions(+)
- **97** deletions(-)
