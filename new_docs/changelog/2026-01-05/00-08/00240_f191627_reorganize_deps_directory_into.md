# [240] refactor: reorganize deps directory into testing/ and runtime/

**Commit**: `f191627` ([`f1916272835998934b0ac51ad8f29df276ba33ca`](https://github.com/licoded/CosyZeroRewrite/commit/f1916272835998934b0ac51ad8f29df276ba33ca))
**Date**: 2026-01-05 00:32:11 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

New structure:
- deps/testing/
  - catch2/     : Testing framework
- deps/runtime/
  - CLI11/      : Command-line parsing library
  - indicators/ : Progress bars and spinners
  - nlohmann/   : JSON library
- deps/spdlog/  : Logging library (unchanged)

Rationale:
- Separates development tools (testing) from runtime dependencies
- Provides clearer organization of external dependencies
- Eliminates duplicate/ambiguous external/ directory

Modified files:
- CMakeLists.txt: Updated Cosy2 include path
- cmake/LibraryTargets.cmake: Updated external -> deps/runtime
- cmake/Tests.cmake: Updated catch2 and external paths
- Directory reorganization: external/CLI, external/indicators,
  external/nlohmann -> deps/runtime/, deps/catch2 -> deps/testing/

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
- `deps/runtime/CLI/App.hpp`
- `deps/runtime/CLI/Argv.hpp`
- `deps/runtime/CLI/CLI.hpp`
- `deps/runtime/CLI/ConfigFwd.hpp`
- `deps/runtime/CLI/Config.hpp`
- `deps/runtime/CLI/Encoding.hpp`
- `deps/runtime/CLI/Error.hpp`
- `deps/runtime/CLI/FormatterFwd.hpp`
- `deps/runtime/CLI/Formatter.hpp`
- `deps/runtime/CLI/impl/App_inl.hpp`
- `deps/runtime/CLI/impl/Argv_inl.hpp`
- `deps/runtime/CLI/impl/Config_inl.hpp`
- `deps/runtime/CLI/impl/Encoding_inl.hpp`
- `deps/runtime/CLI/impl/Formatter_inl.hpp`
- `deps/runtime/CLI/impl/Option_inl.hpp`
- `deps/runtime/CLI/impl/Split_inl.hpp`
- `deps/runtime/CLI/impl/StringTools_inl.hpp`
- `deps/runtime/CLI/impl/Validators_inl.hpp`
- `deps/runtime/CLI/Macros.hpp`
- `deps/runtime/CLI/Option.hpp`
- `deps/runtime/CLI/Split.hpp`
- `deps/runtime/CLI/StringTools.hpp`
- `deps/runtime/CLI/Timer.hpp`
- `deps/runtime/CLI/TypeTools.hpp`
- `deps/runtime/CLI/Validators.hpp`
- `deps/runtime/CLI/Version.hpp`
- `deps/runtime/indicators/block_progress_bar.hpp`
- `deps/runtime/indicators/color.hpp`
- `deps/runtime/indicators/cursor_control.hpp`
- `deps/runtime/indicators/cursor_movement.hpp`
- `deps/runtime/indicators/details/stream_helper.hpp`
- `deps/runtime/indicators/display_width.hpp`
- `deps/runtime/indicators/dynamic_progress.hpp`
- `deps/runtime/indicators/font_style.hpp`
- `deps/runtime/indicators/indeterminate_progress_bar.hpp`
- `deps/runtime/indicators/multi_progress.hpp`
- `deps/runtime/indicators/progress_bar.hpp`
- `deps/runtime/indicators/progress_spinner.hpp`
- `deps/runtime/indicators/progress_type.hpp`
- `deps/runtime/indicators/setting.hpp`
- `deps/runtime/indicators/termcolor.hpp`
- `deps/runtime/indicators/terminal_size.hpp`
- `deps/runtime/nlohmann/json.hpp`
- `deps/testing/catch2/catch.hpp`


### Modified
- `cmake/LibraryTargets.cmake`
- `CMakeLists.txt`
- `cmake/Tests.cmake`


### Deleted
- `deps/catch2/catch.hpp`
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
- `external/nlohmann/json.hpp`


## Stats

- **91** files changed
- **57671** insertions(+)
- **57671** deletions(-)
