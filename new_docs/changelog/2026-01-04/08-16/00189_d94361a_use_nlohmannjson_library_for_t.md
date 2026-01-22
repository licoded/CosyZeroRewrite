# [189] refactor: use nlohmann/json library for trace JSON generation

**Commit**: `d94361a` ([`d94361a350002eebdf5cfc4b9235d47da3feb9bf`](https://github.com/licoded/CosyZeroRewrite/commit/d94361a350002eebdf5cfc4b9235d47da3feb9bf))
**Date**: 2026-01-04 14:50:36 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Download nlohmann/json v3.11.3 single-header library to external/nlohmann/
- Rewrite write_json() to use nlohmann/json instead of manual string concatenation
- Remove old write_stage(), write_sub_step(), write_highlights(), write_state_info() methods
- Add external directory to formula library include path
- state_data is now only included in JSON when non-empty

This fixes JSON parsing issues caused by manual formatting errors.

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
- `external/nlohmann/json.hpp`


### Modified
- `cmake/LibraryTargets.cmake`
- `CMakeLists.txt`
- `include/synthesis/trace_exporter.hpp`
- `src/synthesis/trace_exporter.cpp`


## Stats

- **5** files changed
- **24891** insertions(+)
- **179** deletions(-)
