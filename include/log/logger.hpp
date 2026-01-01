#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
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
        logger_->set_level(level);
    }

    // Flush log
    void flush() {
        logger_->flush();
    }

private:
    Logger() {
        try {
            // Create logs directory if it doesn't exist
            std::filesystem::create_directories("logs");

            // Get current timestamp for log filename
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&time_t);
            char time_buf[64];
            std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &tm);

            std::string log_file = "logs/formula_" + std::string(time_buf) + ".log";

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

        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
    }

    ~Logger() {
        if (logger_) {
            logger_->flush();
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
};

// Convenience macros
#define LOG_TRACE(...) logger::Logger::instance().get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) logger::Logger::instance().get()->debug(__VA_ARGS__)
#define LOG_INFO(...)  logger::Logger::instance().get()->info(__VA_ARGS__)
#define LOG_WARN(...)  logger::Logger::instance().get()->warn(__VA_ARGS__)
#define LOG_ERROR(...) logger::Logger::instance().get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) logger::Logger::instance().get()->critical(__VA_ARGS__)

// Flush log
#define LOG_FLUSH() logger::Logger::instance().flush()

} // namespace logger

#endif // LOGGER_HPP
