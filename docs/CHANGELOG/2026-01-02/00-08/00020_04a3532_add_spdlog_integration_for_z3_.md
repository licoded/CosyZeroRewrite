# [20] Add spdlog integration for Z3 BMC logging

**Commit**: `04a3532` ([`04a353244f0493730138f24b81e8f547e5741cc4`](https://github.com/anthropics/cosy-zero/commit/04a353244f0493730138f24b81e8f547e5741cc4))
**Date**: 2026-01-02 02:03:44 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Features:
- spdlog detection via pkg-config with manual fallback
- Dual output: console (colored, info+) and file (rotating, trace+)
- Log files: logs/formula_YYYYMMDD_HHMMSS.log
- Rotation: 5MB per file, max 3 files
- Conditional compilation: FORMULA_USE_LOGGER

Logger levels:
- TRACE: Time estimation details
- DEBUG: Auto-detected bounds, BMC steps
- INFO: Key operations, results, timing
- WARN: Timeout abortions, inability to determine
- ERROR: Z3 exceptions

Example log output:
[2026-01-02 02:03:17.440] [info] Z3 are_equivalent called: max_bound=-1, timeout=5000ms
[2026-01-02 02:03:17.440] [info] Incremental BMC: auto_detected_bound=1, target_bound=8, x_depth=0, temporal_ops=0
[2026-01-02 02:03:17.452] [info] Bound 2: result=equivalent, time=11.44ms

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `include/log/logger.hpp`
- `logs/.gitkeep`


### Modified
- `CMakeLists.txt`
- `.gitignore`
- `src/formula/formula_z3.cpp`


## Stats

- **5** files changed
- **194** insertions(+)
- **2** deletions(-)
