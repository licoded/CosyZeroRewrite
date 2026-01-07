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
 */
class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    // Get the underlying spdlog logger
    std::shared_ptr<spdlog::logger>& get() { return logger_; }

    // Set log level
    void set_level(spdlog::level::level_enum level) {
        if (logger_) logger_->set_level(level);
    }

    // Flush log
    void flush() {
        if (logger_) logger_->flush();
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

            // Create multi-sink logger
            std::vector<spdlog::sink_ptr> sinks;

            // Console sink (colored)
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

            // Create logger
            logger_ = std::make_shared<spdlog::logger>("formula", sinks.begin(), sinks.end());
            logger_->set_level(spdlog::level::debug);  // Default level
            logger_->flush_on(spdlog::level::warn);   // Auto-flush on warning+

            spdlog::register_logger(logger_);
            spdlog::set_default_logger(logger_);

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
                } catch (...) {
                    // Last resort: use stderr
                    logger_ = nullptr;
                }
            }
        } catch (...) {
            std::cerr << "Unknown error during log initialization, continuing without logging..." << std::endl;
            logger_ = nullptr;
        }
    }

    ~Logger() {
        if (logger_) {
            logger_->flush();
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
};

// Convenience macros (with null check for safety)
// Note: do-while(0) wrapper is a standard C++ macro pattern that makes the macro
// safe to use in if-else statements without breaking control flow.
#define LOG_TRACE(...) do { if (auto lg = logger::Logger::instance().get()) lg->trace(__VA_ARGS__); } while(0)
#define LOG_DEBUG(...) do { if (auto lg = logger::Logger::instance().get()) lg->debug(__VA_ARGS__); } while(0)
#define LOG_INFO(...)  do { if (auto lg = logger::Logger::instance().get()) lg->info(__VA_ARGS__); } while(0)
#define LOG_WARN(...)  do { if (auto lg = logger::Logger::instance().get()) lg->warn(__VA_ARGS__); } while(0)
#define LOG_ERROR(...) do { if (auto lg = logger::Logger::instance().get()) lg->error(__VA_ARGS__); } while(0)
#define LOG_CRITICAL(...) do { if (auto lg = logger::Logger::instance().get()) lg->critical(__VA_ARGS__); } while(0)

// Flush log
#define LOG_FLUSH() do { if (auto lg = logger::Logger::instance().get()) lg->flush(); } while(0)

} // namespace logger

#endif // LOGGER_HPP
