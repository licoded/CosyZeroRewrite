# [252] refactor: reorganize deps directory into external/internal structure

**Commit**: `818b68b` ([`818b68bd94b23585982e72a92e81c8514a1b8952`](https://github.com/licoded/CosyZeroRewrite/commit/818b68bd94b23585982e72a92e81c8514a1b8952))
**Date**: 2026-01-05 08:52:32 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Reorganize deps/ into external/ (third-party) and internal/ (reserved)
- Move all third-party libs: spdlog, catch2, CLI, indicators, nlohmann
- Update CMake include paths: deps/runtime, deps/testing → deps/external
- Remove obsolete benchmark_runner target (file never existed)
- Update deps/README.md with new structure and library details

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## AI Analysis

### 📝 Change Summary
将 `deps/` 目录重组为更清晰的结构：`external/` 存放第三方库，`internal/` 预留给未来内部工具。

### 🔍 Technical Details
- 按来源分类：外部依赖 vs 内部工具
- 所有第三方库统一放在 `deps/external/` 下
- CMake include 路径同步更新

### 📊 Impact Analysis
- 影响范围：CMake 配置文件、依赖目录结构
- 无功能变更，纯重构
- 所有单元测试通过

## Changes

### Added
- `deps/external/catch2/catch.hpp`
- `deps/external/CLI/App.hpp`
- `deps/external/CLI/Argv.hpp`
- `deps/external/CLI/CLI.hpp`
- `deps/external/CLI/ConfigFwd.hpp`
- `deps/external/CLI/Config.hpp`
- `deps/external/CLI/Encoding.hpp`
- `deps/external/CLI/Error.hpp`
- `deps/external/CLI/FormatterFwd.hpp`
- `deps/external/CLI/Formatter.hpp`
- `deps/external/CLI/impl/App_inl.hpp`
- `deps/external/CLI/impl/Argv_inl.hpp`
- `deps/external/CLI/impl/Config_inl.hpp`
- `deps/external/CLI/impl/Encoding_inl.hpp`
- `deps/external/CLI/impl/Formatter_inl.hpp`
- `deps/external/CLI/impl/Option_inl.hpp`
- `deps/external/CLI/impl/Split_inl.hpp`
- `deps/external/CLI/impl/StringTools_inl.hpp`
- `deps/external/CLI/impl/Validators_inl.hpp`
- `deps/external/CLI/Macros.hpp`
- `deps/external/CLI/Option.hpp`
- `deps/external/CLI/Split.hpp`
- `deps/external/CLI/StringTools.hpp`
- `deps/external/CLI/Timer.hpp`
- `deps/external/CLI/TypeTools.hpp`
- `deps/external/CLI/Validators.hpp`
- `deps/external/CLI/Version.hpp`
- `deps/external/indicators/block_progress_bar.hpp`
- `deps/external/indicators/color.hpp`
- `deps/external/indicators/cursor_control.hpp`
- `deps/external/indicators/cursor_movement.hpp`
- `deps/external/indicators/details/stream_helper.hpp`
- `deps/external/indicators/display_width.hpp`
- `deps/external/indicators/dynamic_progress.hpp`
- `deps/external/indicators/font_style.hpp`
- `deps/external/indicators/indeterminate_progress_bar.hpp`
- `deps/external/indicators/multi_progress.hpp`
- `deps/external/indicators/progress_bar.hpp`
- `deps/external/indicators/progress_spinner.hpp`
- `deps/external/indicators/progress_type.hpp`
- `deps/external/indicators/setting.hpp`
- `deps/external/indicators/termcolor.hpp`
- `deps/external/indicators/terminal_size.hpp`
- `deps/external/nlohmann/json.hpp`
- `deps/external/spdlog/async.h`
- `deps/external/spdlog/async_logger.h`
- `deps/external/spdlog/async_logger-inl.h`
- `deps/external/spdlog/cfg/argv.h`
- `deps/external/spdlog/cfg/env.h`
- `deps/external/spdlog/cfg/helpers.h`
- `deps/external/spdlog/cfg/helpers-inl.h`
- `deps/external/spdlog/common.h`
- `deps/external/spdlog/common-inl.h`
- `deps/external/spdlog/details/backtracer.h`
- `deps/external/spdlog/details/backtracer-inl.h`
- `deps/external/spdlog/details/circular_q.h`
- `deps/external/spdlog/details/console_globals.h`
- `deps/external/spdlog/details/file_helper.h`
- `deps/external/spdlog/details/file_helper-inl.h`
- `deps/external/spdlog/details/fmt_helper.h`
- `deps/external/spdlog/details/log_msg_buffer.h`
- `deps/external/spdlog/details/log_msg_buffer-inl.h`
- `deps/external/spdlog/details/log_msg.h`
- `deps/external/spdlog/details/log_msg-inl.h`
- `deps/external/spdlog/details/mpmc_blocking_q.h`
- `deps/external/spdlog/details/null_mutex.h`
- `deps/external/spdlog/details/os.h`
- `deps/external/spdlog/details/os-inl.h`
- `deps/external/spdlog/details/periodic_worker.h`
- `deps/external/spdlog/details/periodic_worker-inl.h`
- `deps/external/spdlog/details/registry.h`
- `deps/external/spdlog/details/registry-inl.h`
- `deps/external/spdlog/details/synchronous_factory.h`
- `deps/external/spdlog/details/tcp_client.h`
- `deps/external/spdlog/details/tcp_client-windows.h`
- `deps/external/spdlog/details/thread_pool.h`
- `deps/external/spdlog/details/thread_pool-inl.h`
- `deps/external/spdlog/details/udp_client.h`
- `deps/external/spdlog/details/udp_client-windows.h`
- `deps/external/spdlog/details/windows_include.h`
- `deps/external/spdlog/fmt/bin_to_hex.h`
- `deps/external/spdlog/fmt/bundled/args.h`
- `deps/external/spdlog/fmt/bundled/chrono.h`
- `deps/external/spdlog/fmt/bundled/color.h`
- `deps/external/spdlog/fmt/bundled/compile.h`
- `deps/external/spdlog/fmt/bundled/core.h`
- `deps/external/spdlog/fmt/bundled/fmt.license.rst`
- `deps/external/spdlog/fmt/bundled/format.h`
- `deps/external/spdlog/fmt/bundled/format-inl.h`
- `deps/external/spdlog/fmt/bundled/locale.h`
- `deps/external/spdlog/fmt/bundled/os.h`
- `deps/external/spdlog/fmt/bundled/ostream.h`
- `deps/external/spdlog/fmt/bundled/printf.h`
- `deps/external/spdlog/fmt/bundled/ranges.h`
- `deps/external/spdlog/fmt/bundled/std.h`
- `deps/external/spdlog/fmt/bundled/xchar.h`
- `deps/external/spdlog/fmt/chrono.h`
- `deps/external/spdlog/fmt/compile.h`
- `deps/external/spdlog/fmt/fmt.h`
- `deps/external/spdlog/fmt/ostr.h`
- `deps/external/spdlog/fmt/ranges.h`
- `deps/external/spdlog/fmt/std.h`
- `deps/external/spdlog/fmt/xchar.h`
- `deps/external/spdlog/formatter.h`
- `deps/external/spdlog/fwd.h`
- `deps/external/spdlog/logger.h`
- `deps/external/spdlog/logger-inl.h`
- `deps/external/spdlog/pattern_formatter.h`
- `deps/external/spdlog/pattern_formatter-inl.h`
- `deps/external/spdlog/sinks/android_sink.h`
- `deps/external/spdlog/sinks/ansicolor_sink.h`
- `deps/external/spdlog/sinks/ansicolor_sink-inl.h`
- `deps/external/spdlog/sinks/base_sink.h`
- `deps/external/spdlog/sinks/base_sink-inl.h`
- `deps/external/spdlog/sinks/basic_file_sink.h`
- `deps/external/spdlog/sinks/basic_file_sink-inl.h`
- `deps/external/spdlog/sinks/callback_sink.h`
- `deps/external/spdlog/sinks/daily_file_sink.h`
- `deps/external/spdlog/sinks/dist_sink.h`
- `deps/external/spdlog/sinks/dup_filter_sink.h`
- `deps/external/spdlog/sinks/hourly_file_sink.h`
- `deps/external/spdlog/sinks/kafka_sink.h`
- `deps/external/spdlog/sinks/mongo_sink.h`
- `deps/external/spdlog/sinks/msvc_sink.h`
- `deps/external/spdlog/sinks/null_sink.h`
- `deps/external/spdlog/sinks/ostream_sink.h`
- `deps/external/spdlog/sinks/qt_sinks.h`
- `deps/external/spdlog/sinks/ringbuffer_sink.h`
- `deps/external/spdlog/sinks/rotating_file_sink.h`
- `deps/external/spdlog/sinks/rotating_file_sink-inl.h`
- `deps/external/spdlog/sinks/sink.h`
- `deps/external/spdlog/sinks/sink-inl.h`
- `deps/external/spdlog/sinks/stdout_color_sinks.h`
- `deps/external/spdlog/sinks/stdout_color_sinks-inl.h`
- `deps/external/spdlog/sinks/stdout_sinks.h`
- `deps/external/spdlog/sinks/stdout_sinks-inl.h`
- `deps/external/spdlog/sinks/syslog_sink.h`
- `deps/external/spdlog/sinks/systemd_sink.h`
- `deps/external/spdlog/sinks/tcp_sink.h`
- `deps/external/spdlog/sinks/udp_sink.h`
- `deps/external/spdlog/sinks/wincolor_sink.h`
- `deps/external/spdlog/sinks/wincolor_sink-inl.h`
- `deps/external/spdlog/sinks/win_eventlog_sink.h`
- `deps/external/spdlog/spdlog.h`
- `deps/external/spdlog/spdlog-inl.h`
- `deps/external/spdlog/stopwatch.h`
- `deps/external/spdlog/tweakme.h`
- `deps/external/spdlog/version.h`


### Modified
- `cmake/LibraryTargets.cmake`
- `CMakeLists.txt`
- `cmake/Tests.cmake`
- `deps/README.md`


### Deleted
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
- `deps/spdlog/async.h`
- `deps/spdlog/async_logger.h`
- `deps/spdlog/async_logger-inl.h`
- `deps/spdlog/cfg/argv.h`
- `deps/spdlog/cfg/env.h`
- `deps/spdlog/cfg/helpers.h`
- `deps/spdlog/cfg/helpers-inl.h`
- `deps/spdlog/common.h`
- `deps/spdlog/common-inl.h`
- `deps/spdlog/details/backtracer.h`
- `deps/spdlog/details/backtracer-inl.h`
- `deps/spdlog/details/circular_q.h`
- `deps/spdlog/details/console_globals.h`
- `deps/spdlog/details/file_helper.h`
- `deps/spdlog/details/file_helper-inl.h`
- `deps/spdlog/details/fmt_helper.h`
- `deps/spdlog/details/log_msg_buffer.h`
- `deps/spdlog/details/log_msg_buffer-inl.h`
- `deps/spdlog/details/log_msg.h`
- `deps/spdlog/details/log_msg-inl.h`
- `deps/spdlog/details/mpmc_blocking_q.h`
- `deps/spdlog/details/null_mutex.h`
- `deps/spdlog/details/os.h`
- `deps/spdlog/details/os-inl.h`
- `deps/spdlog/details/periodic_worker.h`
- `deps/spdlog/details/periodic_worker-inl.h`
- `deps/spdlog/details/registry.h`
- `deps/spdlog/details/registry-inl.h`
- `deps/spdlog/details/synchronous_factory.h`
- `deps/spdlog/details/tcp_client.h`
- `deps/spdlog/details/tcp_client-windows.h`
- `deps/spdlog/details/thread_pool.h`
- `deps/spdlog/details/thread_pool-inl.h`
- `deps/spdlog/details/udp_client.h`
- `deps/spdlog/details/udp_client-windows.h`
- `deps/spdlog/details/windows_include.h`
- `deps/spdlog/fmt/bin_to_hex.h`
- `deps/spdlog/fmt/bundled/args.h`
- `deps/spdlog/fmt/bundled/chrono.h`
- `deps/spdlog/fmt/bundled/color.h`
- `deps/spdlog/fmt/bundled/compile.h`
- `deps/spdlog/fmt/bundled/core.h`
- `deps/spdlog/fmt/bundled/fmt.license.rst`
- `deps/spdlog/fmt/bundled/format.h`
- `deps/spdlog/fmt/bundled/format-inl.h`
- `deps/spdlog/fmt/bundled/locale.h`
- `deps/spdlog/fmt/bundled/os.h`
- `deps/spdlog/fmt/bundled/ostream.h`
- `deps/spdlog/fmt/bundled/printf.h`
- `deps/spdlog/fmt/bundled/ranges.h`
- `deps/spdlog/fmt/bundled/std.h`
- `deps/spdlog/fmt/bundled/xchar.h`
- `deps/spdlog/fmt/chrono.h`
- `deps/spdlog/fmt/compile.h`
- `deps/spdlog/fmt/fmt.h`
- `deps/spdlog/fmt/ostr.h`
- `deps/spdlog/fmt/ranges.h`
- `deps/spdlog/fmt/std.h`
- `deps/spdlog/fmt/xchar.h`
- `deps/spdlog/formatter.h`
- `deps/spdlog/fwd.h`
- `deps/spdlog/logger.h`
- `deps/spdlog/logger-inl.h`
- `deps/spdlog/pattern_formatter.h`
- `deps/spdlog/pattern_formatter-inl.h`
- `deps/spdlog/sinks/android_sink.h`
- `deps/spdlog/sinks/ansicolor_sink.h`
- `deps/spdlog/sinks/ansicolor_sink-inl.h`
- `deps/spdlog/sinks/base_sink.h`
- `deps/spdlog/sinks/base_sink-inl.h`
- `deps/spdlog/sinks/basic_file_sink.h`
- `deps/spdlog/sinks/basic_file_sink-inl.h`
- `deps/spdlog/sinks/callback_sink.h`
- `deps/spdlog/sinks/daily_file_sink.h`
- `deps/spdlog/sinks/dist_sink.h`
- `deps/spdlog/sinks/dup_filter_sink.h`
- `deps/spdlog/sinks/hourly_file_sink.h`
- `deps/spdlog/sinks/kafka_sink.h`
- `deps/spdlog/sinks/mongo_sink.h`
- `deps/spdlog/sinks/msvc_sink.h`
- `deps/spdlog/sinks/null_sink.h`
- `deps/spdlog/sinks/ostream_sink.h`
- `deps/spdlog/sinks/qt_sinks.h`
- `deps/spdlog/sinks/ringbuffer_sink.h`
- `deps/spdlog/sinks/rotating_file_sink.h`
- `deps/spdlog/sinks/rotating_file_sink-inl.h`
- `deps/spdlog/sinks/sink.h`
- `deps/spdlog/sinks/sink-inl.h`
- `deps/spdlog/sinks/stdout_color_sinks.h`
- `deps/spdlog/sinks/stdout_color_sinks-inl.h`
- `deps/spdlog/sinks/stdout_sinks.h`
- `deps/spdlog/sinks/stdout_sinks-inl.h`
- `deps/spdlog/sinks/syslog_sink.h`
- `deps/spdlog/sinks/systemd_sink.h`
- `deps/spdlog/sinks/tcp_sink.h`
- `deps/spdlog/sinks/udp_sink.h`
- `deps/spdlog/sinks/wincolor_sink.h`
- `deps/spdlog/sinks/wincolor_sink-inl.h`
- `deps/spdlog/sinks/win_eventlog_sink.h`
- `deps/spdlog/spdlog.h`
- `deps/spdlog/spdlog-inl.h`
- `deps/spdlog/stopwatch.h`
- `deps/spdlog/tweakme.h`
- `deps/spdlog/version.h`
- `deps/testing/catch2/catch.hpp`


## Stats

- **300** files changed
- **84249** insertions(+)
- **84230** deletions(-)
