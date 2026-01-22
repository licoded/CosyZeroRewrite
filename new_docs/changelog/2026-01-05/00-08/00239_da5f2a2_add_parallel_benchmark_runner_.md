# [239] feat: add parallel benchmark runner with CLI11 and indicators

**Commit**: `da5f2a2` ([`da5f2a2c5321d2ff70ba056e39e1982644af1a01`](https://github.com/licoded/CosyZeroRewrite/commit/da5f2a2c5321d2ff70ba056e39e1982644af1a01))
**Date**: 2026-01-05 00:30:24 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Added libraries:
- CLI11 v2.4.2: Command-line argument parsing
- indicators v2.3: Progress bars and spinners

New components:
- include/parallel/thread_pool.hpp: Lightweight thread pool using std::thread
  with worker spawning and concurrency limiting
- tests/bench/benchmark.cpp: Rewritten with parallel execution support

Features:
- -j, --jobs <N>: Control parallelism (default: CPU count)
- -v, --verbose: Print all cases
- -q, --quiet: Only print summary
- --no-progress: Disable progress bar
- --no-active: Don't show active tasks

Performance note:
- For small tasks (parsing ~0.03ms), serial execution is faster
- The parallel infrastructure will benefit larger tasks (synthesis)
- Progress bar now thread-safe with mutex protection

Modified files:
- cmake/Tests.cmake: Added external/include to test targets
- include/parallel/thread_pool.hpp: New thread pool implementation
- tests/bench/benchmark.cpp: Complete rewrite with CLI11 and parallel support

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
- `external/CLI/App.hpp`
- `external/CLI/Argv.hpp`
- `external/CLI/CLI.hpp`
- `external/CLI/ConfigFwd.hpp`
- `external/CLI/Config.hpp`
- `external/CLI/Encoding.hpp`
- `external/CLI/Error.hpp`
- `external/CLI/FormatterFwd.hpp`
- `external/CLI/Formatter.hpp`
- `external/CLI/impl/App_inl.hpp`
- `external/CLI/impl/Argv_inl.hpp`
- `external/CLI/impl/Config_inl.hpp`
- `external/CLI/impl/Encoding_inl.hpp`
- `external/CLI/impl/Formatter_inl.hpp`
- `external/CLI/impl/Option_inl.hpp`
- `external/CLI/impl/Split_inl.hpp`
- `external/CLI/impl/StringTools_inl.hpp`
- `external/CLI/impl/Validators_inl.hpp`
- `external/CLI/Macros.hpp`
- `external/CLI/Option.hpp`
- `external/CLI/Split.hpp`
- `external/CLI/StringTools.hpp`
- `external/CLI/Timer.hpp`
- `external/CLI/TypeTools.hpp`
- `external/CLI/Validators.hpp`
- `external/CLI/Version.hpp`
- `external/indicators/block_progress_bar.hpp`
- `external/indicators/color.hpp`
- `external/indicators/cursor_control.hpp`
- `external/indicators/cursor_movement.hpp`
- `external/indicators/details/stream_helper.hpp`
- `external/indicators/display_width.hpp`
- `external/indicators/dynamic_progress.hpp`
- `external/indicators/font_style.hpp`
- `external/indicators/indeterminate_progress_bar.hpp`
- `external/indicators/multi_progress.hpp`
- `external/indicators/progress_bar.hpp`
- `external/indicators/progress_spinner.hpp`
- `external/indicators/progress_type.hpp`
- `external/indicators/setting.hpp`
- `external/indicators/termcolor.hpp`
- `external/indicators/terminal_size.hpp`
- `include/parallel/thread_pool.hpp`


### Modified
- `cmake/Tests.cmake`
- `tests/bench/benchmark.cpp`


## Stats

- **45** files changed
- **15508** insertions(+)
- **215** deletions(-)
