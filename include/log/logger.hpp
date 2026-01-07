#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

namespace logger {

/**
 * @brief Simple logger wrapper using spdlog
 *
 * Features:
 * - Console output with colors
 * - File output with rotation (5MB per file, max 3 files)
 * - Timestamp on each message
 * - Log level: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
 * - Separate "output" logger for user-facing output (no prefix)
 */
class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    // Get the underlying spdlog logger (for internal logging)
    std::shared_ptr<spdlog::logger>& get() { return logger_; }

    // Get the output logger (for user-facing output, no prefix)
    std::shared_ptr<spdlog::logger>& output() { return output_; }

    // Set log level
    void set_level(spdlog::level::level_enum level) {
        if (logger_) logger_->set_level(level);
    }

    // Flush log
    void flush() {
        if (logger_) logger_->flush();
        if (output_) output_->flush();
    }

    /**
     * @brief Explicitly initialize logger with log directory creation
     * @return true if initialization succeeded (with or without file logging)
     *
     * Call this at the start of main() to ensure log directory exists.
     * If file logging fails, falls back to console-only logging.
     */
    static bool initialize() {
        // Just trigger the singleton initialization
        instance();
        return instance().logger_ != nullptr;
    }

private:
    Logger() {
        try {
            // Get current time for directory structure
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm_info = std::localtime(&time_t);
            int hour = tm_info->tm_hour;

            // Determine period: 01-morning(6-12), 02-afternoon(12-18), 03-evening(18-24), 04-night(0-6)
            std::string period;
            if (hour >= 6 && hour < 12) period = "01-morning";
            else if (hour >= 12 && hour < 18) period = "02-afternoon";
            else if (hour >= 18) period = "03-evening";
            else period = "04-night";

            // Create log path: logs/formula/YYYY-MM-DD/period/formula_YYYYMMDD_HHMMSS.log
            std::ostringstream log_path;
            log_path << "logs/formula/"
                      << std::put_time(tm_info, "%Y-%m-%d")
                      << "/" << period
                      << "/formula_"
                      << std::put_time(tm_info, "%Y%m%d_%H%M%S")
                      << ".log";

            // Create directory
            std::filesystem::create_directories(std::filesystem::path(log_path.str()).parent_path());

            std::string log_file = log_path.str();

            //==================================================================
            // 1. Internal logging logger (with prefix for debugging)
            //==================================================================
            std::vector<spdlog::sink_ptr> sinks;

            // Console sink (colored, with prefix)
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::info);
            console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
            sinks.push_back(console_sink);

            // File sink (rotating)
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file, 1024 * 1024 * 5, 3);  // 5MB per file, max 3 files
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
            sinks.push_back(file_sink);

            // Create internal logger
            logger_ = std::make_shared<spdlog::logger>("formula", sinks.begin(), sinks.end());
            logger_->set_level(spdlog::level::debug);  // Default level
            logger_->flush_on(spdlog::level::warn);   // Auto-flush on warning+

            spdlog::register_logger(logger_);
            spdlog::set_default_logger(logger_);

            //==================================================================
            // 2. User-facing output logger (no prefix, clean output)
            //==================================================================
            std::vector<spdlog::sink_ptr> output_sinks;

            // Console sink (no prefix)
            auto output_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            output_console_sink->set_level(spdlog::level::info);
            output_console_sink->set_pattern("%v");  // No prefix, just the message
            output_sinks.push_back(output_console_sink);

            // Share the same file sink (for debugging user output in logs)
            output_sinks.push_back(file_sink);

            // Create output logger
            output_ = std::make_shared<spdlog::logger>("output", output_sinks.begin(), output_sinks.end());
            output_->set_level(spdlog::level::info);
            output_->flush_on(spdlog::level::info);  // Always flush user output

            spdlog::register_logger(output_);

        } catch (const std::exception& ex) {
            // Catch all exceptions including filesystem_error
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            std::cerr << "Continuing without file logging..." << std::endl;
            // Fallback to console-only logger
            if (!logger_) {
                try {
                    std::vector<spdlog::sink_ptr> sinks;
                    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                    console_sink->set_level(spdlog::level::info);
                    console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
                    sinks.push_back(console_sink);
                    logger_ = std::make_shared<spdlog::logger>("formula", sinks.begin(), sinks.end());
                    logger_->set_level(spdlog::level::debug);
                    logger_->flush_on(spdlog::level::warn);
                    spdlog::register_logger(logger_);
                    spdlog::set_default_logger(logger_);

                    // Create minimal output logger
                    std::vector<spdlog::sink_ptr> output_sinks;
                    auto output_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                    output_sink->set_level(spdlog::level::info);
                    output_sink->set_pattern("%v");
                    output_sinks.push_back(output_sink);
                    output_ = std::make_shared<spdlog::logger>("output", output_sinks.begin(), output_sinks.end());
                    spdlog::register_logger(output_);
                } catch (...) {
                    // Last resort: use stderr
                    logger_ = nullptr;
                    output_ = nullptr;
                }
            }
        } catch (...) {
            std::cerr << "Unknown error during log initialization, continuing without logging..." << std::endl;
            logger_ = nullptr;
            output_ = nullptr;
        }
    }

    ~Logger() {
        if (logger_) {
            logger_->flush();
        }
        if (output_) {
            output_->flush();
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> output_;
};

//==============================================================================
// Convenience macros (with null check for safety)
// Note: do-while(0) wrapper is a standard C++ macro pattern that makes the macro
// safe to use in if-else statements without breaking control flow.
//==============================================================================

// Internal logging macros (with timestamp and level prefix)
#define LOG_TRACE(...) do { if (auto lg = logger::Logger::instance().get()) lg->trace(__VA_ARGS__); } while(0)
#define LOG_DEBUG(...) do { if (auto lg = logger::Logger::instance().get()) lg->debug(__VA_ARGS__); } while(0)
#define LOG_INFO(...)  do { if (auto lg = logger::Logger::instance().get()) lg->info(__VA_ARGS__); } while(0)
#define LOG_WARN(...)  do { if (auto lg = logger::Logger::instance().get()) lg->warn(__VA_ARGS__); } while(0)
#define LOG_ERROR(...) do { if (auto lg = logger::Logger::instance().get()) lg->error(__VA_ARGS__); } while(0)
#define LOG_CRITICAL(...) do { if (auto lg = logger::Logger::instance().get()) lg->critical(__VA_ARGS__); } while(0)

// User-facing output macro (no prefix, just the message)
// This logs to both console (clean) and file (with timestamp for debugging)
#define LOG_OUTPUT(...) do { \
    if (auto lg = logger::Logger::instance().output()) { \
        lg->info(__VA_ARGS__); \
    } \
} while(0)

// Flush log
#define LOG_FLUSH() do { \
    if (auto lg = logger::Logger::instance().get()) lg->flush(); \
    if (auto ol = logger::Logger::instance().output()) ol->flush(); \
} while(0)

} // namespace logger

#endif // LOGGER_HPP
