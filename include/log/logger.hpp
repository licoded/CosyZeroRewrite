#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/cfg/env.h>  // for load_env_levels()
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
        spdlog::cfg::load_env_levels();  // Load log levels from SPDLOG_LEVEL env var
        return inst;
    }

    // Get the underlying spdlog logger (for internal logging)
    std::shared_ptr<spdlog::logger>& logger() { return logger_; }

    // Get the output logger (for user-facing output, no prefix)
    std::shared_ptr<spdlog::logger>& no_perfix_logger() { return no_perfix_logger_; }

    // Set log level
    void set_level(spdlog::level::level_enum level) {
        if (logger_) logger_->set_level(level);
    }

    // Flush log
    void flush() {
        if (logger_) logger_->flush();
        if (no_perfix_logger_) no_perfix_logger_->flush();
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

            // Create log path: logs/formula/YYYY-MM-DD/formula_YYYYMMDD_HHMMSS.log
            std::ostringstream log_path;
            log_path << "logs/formula/"
                      << std::put_time(tm_info, "%Y-%m-%d")
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
            std::vector<spdlog::sink_ptr> nop_sinks;

            // Console sink (no prefix)
            auto nop_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            nop_console_sink->set_level(spdlog::level::info);
            nop_console_sink->set_pattern("%v");  // No prefix, just the message
            nop_sinks.push_back(nop_console_sink);

            // Share the same file sink (for debugging user output in logs)
            nop_sinks.push_back(file_sink);

            // Create output logger
            no_perfix_logger_ = std::make_shared<spdlog::logger>("output", nop_sinks.begin(), nop_sinks.end());
            no_perfix_logger_->set_level(spdlog::level::info);
            no_perfix_logger_->flush_on(spdlog::level::info);  // Always flush user output

            spdlog::register_logger(no_perfix_logger_);

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
                    std::vector<spdlog::sink_ptr> nop_sinks;
                    auto nop_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                    nop_console_sink->set_level(spdlog::level::info);
                    nop_console_sink->set_pattern("%v");
                    nop_sinks.push_back(nop_console_sink);
                    no_perfix_logger_ = std::make_shared<spdlog::logger>("output", nop_sinks.begin(), nop_sinks.end());
                    spdlog::register_logger(no_perfix_logger_);
                } catch (...) {
                    // Last resort: use stderr
                    logger_ = nullptr;
                    no_perfix_logger_ = nullptr;
                }
            }
        } catch (...) {
            std::cerr << "Unknown error during log initialization, continuing without logging..." << std::endl;
            logger_ = nullptr;
            no_perfix_logger_ = nullptr;
        }
    }

    ~Logger() {
        if (logger_) {
            logger_->flush();
        }
        if (no_perfix_logger_) {
            no_perfix_logger_->flush();
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::logger> no_perfix_logger_;
};

//==============================================================================
// Convenience macros (with null check for safety)
// Note: do-while(0) wrapper is a standard C++ macro pattern that makes the macro
// safe to use in if-else statements without breaking control flow.
//==============================================================================

// Internal logging macros (with timestamp and level prefix)
#define LOG_TRACE(...) do { if (auto lg = logger::Logger::instance().logger()) lg->trace(__VA_ARGS__); } while(0)
#define LOG_DEBUG(...) do { if (auto lg = logger::Logger::instance().logger()) lg->debug(__VA_ARGS__); } while(0)
#define LOG_INFO(...)  do { if (auto lg = logger::Logger::instance().logger()) lg->info(__VA_ARGS__); } while(0)
#define LOG_WARN(...)  do { if (auto lg = logger::Logger::instance().logger()) lg->warn(__VA_ARGS__); } while(0)
#define LOG_ERROR(...) do { if (auto lg = logger::Logger::instance().logger()) lg->error(__VA_ARGS__); } while(0)
#define LOG_CRITICAL(...) do { if (auto lg = logger::Logger::instance().logger()) lg->critical(__VA_ARGS__); } while(0)

// User-facing output macro (no prefix, just the message)
// This logs to both console (clean) and file (with timestamp for debugging)
#define NOP_LOG_INFO(...) do { if (auto lg = logger::Logger::instance().no_perfix_logger()) { lg->info(__VA_ARGS__); } } while(0)
#define NOP_LOG_ERROR(...) do { if (auto lg = logger::Logger::instance().no_perfix_logger()) { lg->info(__VA_ARGS__); } } while(0)

// Flush log
#define LOG_FLUSH() do { \
    if (auto lg = logger::Logger::instance().logger()) lg->flush(); \
    if (auto ol = logger::Logger::instance().no_perfix_logger()) ol->flush(); \
} while(0)

} // namespace logger

#endif // LOGGER_HPP
