#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/cfg/env.h>  // for load_env_levels()
#include <spdlog/fmt/fmt.h>  // for fmt::format
#include <filesystem>
#include <chrono>
#include <cstdio>    // for std::strftime
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
    //==========================================================================
    // Helper functions for logger construction
    //==========================================================================

    /**
     * @brief Generate log file path with timestamp
     * @return Path like "logs/formula/YYYY-MM-DD/formula_YYYYMMDD_HHMMSS.log"
     */
    static std::string generate_log_path() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_info = std::localtime(&time_t);

        char date_buf[16];
        char time_buf[16];
        std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm_info);
        std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", tm_info);

        return fmt::format("logs/formula/{}/formula_{}.log", date_buf, time_buf);
    }

    /**
     * @brief Create console sink with standard pattern
     * @return Configured console sink
     */
    static spdlog::sink_ptr create_console_sink() {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_level(spdlog::level::trace);
        sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        return sink;
    }

    /**
     * @brief Create console sink for user output (no prefix)
     * @return Configured console sink with clean pattern
     */
    static spdlog::sink_ptr create_nop_console_sink() {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_level(spdlog::level::trace);
        sink->set_pattern("%v");  // No prefix, just the message
        return sink;
    }

    /**
     * @brief Create file sink with rotation
     * @param log_file Path to log file
     * @return Configured file sink
     */
    static spdlog::sink_ptr create_file_sink(const std::string& log_file) {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 1024 * 1024 * 5, 3);  // 5MB per file, max 3 files
        sink->set_level(spdlog::level::trace);
        sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        return sink;
    }

    /**
     * @brief Create main logger with console and file sinks
     * @param log_file Path to log file
     * @return Configured main logger
     */
    static std::shared_ptr<spdlog::logger> create_logger(const std::string& log_file) {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(create_console_sink());
        sinks.push_back(create_file_sink(log_file));

        auto logger = std::make_shared<spdlog::logger>("formula", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::warn);

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        return logger;
    }

    /**
     * @brief Create output logger for user-facing messages
     * @param file_sink Shared file sink (for debugging user output)
     * @return Configured output logger
     */
    static std::shared_ptr<spdlog::logger> create_nop_logger(spdlog::sink_ptr file_sink) {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(create_nop_console_sink());
        sinks.push_back(file_sink);  // Share file sink for debugging

        auto logger = std::make_shared<spdlog::logger>("output", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);  // Always flush user output

        spdlog::register_logger(logger);
        return logger;
    }

    /**
     * @brief Fallback to console-only logging when file logging fails
     */
    void fallback_to_console_only() {
        try {
            // Create minimal main logger
            std::vector<spdlog::sink_ptr> sinks;
            sinks.push_back(create_console_sink());
            logger_ = std::make_shared<spdlog::logger>("formula", sinks.begin(), sinks.end());
            logger_->set_level(spdlog::level::debug);
            logger_->flush_on(spdlog::level::warn);
            spdlog::register_logger(logger_);
            spdlog::set_default_logger(logger_);

            // Create minimal output logger
            std::vector<spdlog::sink_ptr> nop_sinks;
            nop_sinks.push_back(create_nop_console_sink());
            no_perfix_logger_ = std::make_shared<spdlog::logger>("output", nop_sinks.begin(), nop_sinks.end());
            spdlog::register_logger(no_perfix_logger_);
        } catch (...) {
            // Last resort: use stderr
            logger_ = nullptr;
            no_perfix_logger_ = nullptr;
        }
    }

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    Logger() {
        try {
            std::string log_file = generate_log_path();
            std::filesystem::create_directories(std::filesystem::path(log_file).parent_path());

            logger_ = create_logger(log_file);
            no_perfix_logger_ = create_nop_logger(logger_->sinks()[1]);  // file_sink
        } catch (const std::exception& ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            std::cerr << "Continuing without file logging..." << std::endl;
            fallback_to_console_only();
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
#define NOP_LOG_ERROR(...) do { if (auto lg = logger::Logger::instance().no_perfix_logger()) { lg->error(__VA_ARGS__); } } while(0)

// Flush log
#define LOG_FLUSH() do { \
    if (auto lg = logger::Logger::instance().logger()) lg->flush(); \
    if (auto ol = logger::Logger::instance().no_perfix_logger()) ol->flush(); \
} while(0)

} // namespace logger

#endif // LOGGER_HPP
