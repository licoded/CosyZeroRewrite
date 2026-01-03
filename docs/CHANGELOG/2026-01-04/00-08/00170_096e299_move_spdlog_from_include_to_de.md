# [170] refactor: move spdlog from include/ to deps/

**Commit**: `096e299` ([`096e2990c840f431cecfab5ff9b13d0d753c8b80`](https://github.com/licoded/CosyZeroRewrite/commit/096e2990c840f431cecfab5ff9b13d0d753c8b80))
**Date**: 2026-01-04 01:25:15 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Unified dependency management:
- include/spdlog/ → deps/spdlog/
- Updated cmake/Dependencies.cmake to reference deps/spdlog
- Updated deps/README.md with spdlog information

Now all third-party dependencies are in deps/:
- deps/catch2/  - Test framework
- deps/spdlog/  - Logging library

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


### Modified
- `cmake/Dependencies.cmake`
- `deps/README.md`


### Deleted
- `include/spdlog/async.h`
- `include/spdlog/async_logger.h`
- `include/spdlog/async_logger-inl.h`
- `include/spdlog/cfg/argv.h`
- `include/spdlog/cfg/env.h`
- `include/spdlog/cfg/helpers.h`
- `include/spdlog/cfg/helpers-inl.h`
- `include/spdlog/common.h`
- `include/spdlog/common-inl.h`
- `include/spdlog/details/backtracer.h`
- `include/spdlog/details/backtracer-inl.h`
- `include/spdlog/details/circular_q.h`
- `include/spdlog/details/console_globals.h`
- `include/spdlog/details/file_helper.h`
- `include/spdlog/details/file_helper-inl.h`
- `include/spdlog/details/fmt_helper.h`
- `include/spdlog/details/log_msg_buffer.h`
- `include/spdlog/details/log_msg_buffer-inl.h`
- `include/spdlog/details/log_msg.h`
- `include/spdlog/details/log_msg-inl.h`
- `include/spdlog/details/mpmc_blocking_q.h`
- `include/spdlog/details/null_mutex.h`
- `include/spdlog/details/os.h`
- `include/spdlog/details/os-inl.h`
- `include/spdlog/details/periodic_worker.h`
- `include/spdlog/details/periodic_worker-inl.h`
- `include/spdlog/details/registry.h`
- `include/spdlog/details/registry-inl.h`
- `include/spdlog/details/synchronous_factory.h`
- `include/spdlog/details/tcp_client.h`
- `include/spdlog/details/tcp_client-windows.h`
- `include/spdlog/details/thread_pool.h`
- `include/spdlog/details/thread_pool-inl.h`
- `include/spdlog/details/udp_client.h`
- `include/spdlog/details/udp_client-windows.h`
- `include/spdlog/details/windows_include.h`
- `include/spdlog/fmt/bin_to_hex.h`
- `include/spdlog/fmt/bundled/args.h`
- `include/spdlog/fmt/bundled/chrono.h`
- `include/spdlog/fmt/bundled/color.h`
- `include/spdlog/fmt/bundled/compile.h`
- `include/spdlog/fmt/bundled/core.h`
- `include/spdlog/fmt/bundled/fmt.license.rst`
- `include/spdlog/fmt/bundled/format.h`
- `include/spdlog/fmt/bundled/format-inl.h`
- `include/spdlog/fmt/bundled/locale.h`
- `include/spdlog/fmt/bundled/os.h`
- `include/spdlog/fmt/bundled/ostream.h`
- `include/spdlog/fmt/bundled/printf.h`
- `include/spdlog/fmt/bundled/ranges.h`
- `include/spdlog/fmt/bundled/std.h`
- `include/spdlog/fmt/bundled/xchar.h`
- `include/spdlog/fmt/chrono.h`
- `include/spdlog/fmt/compile.h`
- `include/spdlog/fmt/fmt.h`
- `include/spdlog/fmt/ostr.h`
- `include/spdlog/fmt/ranges.h`
- `include/spdlog/fmt/std.h`
- `include/spdlog/fmt/xchar.h`
- `include/spdlog/formatter.h`
- `include/spdlog/fwd.h`
- `include/spdlog/logger.h`
- `include/spdlog/logger-inl.h`
- `include/spdlog/pattern_formatter.h`
- `include/spdlog/pattern_formatter-inl.h`
- `include/spdlog/sinks/android_sink.h`
- `include/spdlog/sinks/ansicolor_sink.h`
- `include/spdlog/sinks/ansicolor_sink-inl.h`
- `include/spdlog/sinks/base_sink.h`
- `include/spdlog/sinks/base_sink-inl.h`
- `include/spdlog/sinks/basic_file_sink.h`
- `include/spdlog/sinks/basic_file_sink-inl.h`
- `include/spdlog/sinks/callback_sink.h`
- `include/spdlog/sinks/daily_file_sink.h`
- `include/spdlog/sinks/dist_sink.h`
- `include/spdlog/sinks/dup_filter_sink.h`
- `include/spdlog/sinks/hourly_file_sink.h`
- `include/spdlog/sinks/kafka_sink.h`
- `include/spdlog/sinks/mongo_sink.h`
- `include/spdlog/sinks/msvc_sink.h`
- `include/spdlog/sinks/null_sink.h`
- `include/spdlog/sinks/ostream_sink.h`
- `include/spdlog/sinks/qt_sinks.h`
- `include/spdlog/sinks/ringbuffer_sink.h`
- `include/spdlog/sinks/rotating_file_sink.h`
- `include/spdlog/sinks/rotating_file_sink-inl.h`
- `include/spdlog/sinks/sink.h`
- `include/spdlog/sinks/sink-inl.h`
- `include/spdlog/sinks/stdout_color_sinks.h`
- `include/spdlog/sinks/stdout_color_sinks-inl.h`
- `include/spdlog/sinks/stdout_sinks.h`
- `include/spdlog/sinks/stdout_sinks-inl.h`
- `include/spdlog/sinks/syslog_sink.h`
- `include/spdlog/sinks/systemd_sink.h`
- `include/spdlog/sinks/tcp_sink.h`
- `include/spdlog/sinks/udp_sink.h`
- `include/spdlog/sinks/wincolor_sink.h`
- `include/spdlog/sinks/wincolor_sink-inl.h`
- `include/spdlog/sinks/win_eventlog_sink.h`
- `include/spdlog/spdlog.h`
- `include/spdlog/spdlog-inl.h`
- `include/spdlog/stopwatch.h`
- `include/spdlog/tweakme.h`
- `include/spdlog/version.h`


## Stats

- **210** files changed
- **26543** insertions(+)
- **26535** deletions(-)
