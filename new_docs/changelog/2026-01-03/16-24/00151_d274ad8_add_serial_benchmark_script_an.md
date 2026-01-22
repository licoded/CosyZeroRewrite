# [151] docs: add serial benchmark script and update test report

**Commit**: `d274ad8` ([`d274ad8f25036375886bae9ed97d11f02d9691c5`](https://github.com/licoded/CosyZeroRewrite/commit/d274ad8f25036375886bae9ed97d11f02d9691c5))
**Date**: 2026-01-03 23:16:47 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added scripts/serial_benchmark.sh:
- Runs benchmarks one formula at a time (serial mode)
- Avoids concurrency issues that caused timeouts
- Results saved to results/benchmark/serial/

Updated CLAUDE.md:
- Added note about Benchmark timeout solutions
- Use serial mode for testing when encountering timeouts

Updated TEST_REPORT:
- Added serial benchmark test results
- Documented Cosy2 exit code issue
- f100 returns UNREALIZABLE vs expected Realizable

Test Summary:
- Unit Tests: 7/9 passed (77.8%)
- Serial Benchmark: 2/5 had issues (exit codes, timeouts)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
<!-- TODO: Add a brief summary of the change in Chinese or English -->

### 🔍 Technical Details
<!-- Optional: Add technical details, root cause, or implementation notes -->

### 📊 Impact Analysis
<!-- Optional: Add impact scope, affected components, or performance notes -->

## Changes

### Added
- `results/benchmark/serial/2026-01-03/serial_benchmark_20260103_231103.csv`
- `results/benchmark/serial/2026-01-03/serial_benchmark_20260103_231148.csv`
- `scripts/serial_benchmark.sh`


### Modified
- `CLAUDE.md`
- `docs/TEST_REPORT_2026-01-03.md`


## Stats

- **5** files changed
- **230** insertions(+)
- **13** deletions(-)
