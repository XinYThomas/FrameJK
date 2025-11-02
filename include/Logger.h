//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_LOGGER_H
#define FRAME_LOGGER_H

#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>

#define LOG(level) \
    SpdLogger::GetInstance()->createLogMessage(stringToLogLevel(#level), __FILE_NAME__, __LINE__)


enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

inline LogLevel stringToLogLevel(const string& level) {
    if (level == "DEBUG") {
        return LogLevel::DEBUG;
    } else if (level == "INFO") {
        return LogLevel::INFO;
    } else if (level == "WARN") {
        return LogLevel::WARN;
    } else if (level == "ERROR") {
        return LogLevel::ERROR;
    } else if (level == "FATAL") {
        return LogLevel::FATAL;
    }
    return LogLevel::FATAL;
}

// 日志配置项
struct LoggerConfig {
    std::string logPath;
    LogLevel minLogLevel;
    size_t maxFileSize;            // 单个日志文件最大大小（字节）
    size_t maxFiles;               // 最大日志文件数量
    bool asyncMode;                // 是否使用异步模式
    bool consoleOutput;            // 是否输出到控制台
    std::string pattern;           // 日志格式模式

    LoggerConfig() :
        minLogLevel(LogLevel::DEBUG),
        maxFileSize(5 * 1024 * 1024),  // 默认5MB
        maxFiles(10),
        asyncMode(true),
        consoleOutput(true),
        pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v") {}
};

class SpdLogger;

// 输出一个日志类
class LogMessage {
public:
    LogMessage(SpdLogger* loggerMethod, const LogLevel level,
        const char* file = __FILE_NAME__, const int line = __LINE__) :
        loggerMethod_(loggerMethod), level_(level){
        stream_ << "[" << file << ":" << line << "] ";
    }

    ~LogMessage();

    template <typename T>
    LogMessage& operator<<(const T& msg) {
        stream_ << msg;
        return *this;
    }

    std::string str() const {return stream_.str();}
    LogLevel level() const {return level_;}

private:
    SpdLogger* loggerMethod_;
    std::ostringstream stream_;
    LogLevel level_;
};

class SpdLogger {
public:
    static SpdLogger *GetInstance() {
        if (instance == nullptr) {
            instance = new SpdLogger();
        }
        return instance;
    }

    ~SpdLogger() {
        if (instance != nullptr) {
            flush();
        }
    }

    void initialize() {
        std::vector<spdlog::sink_ptr> sinks;

        // 文件
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            conf_->logPath,
            conf_->maxFileSize,
            conf_->maxFiles
        );
        sinks.push_back(file_sink);
        // 控制台
        if (conf_->consoleOutput) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sinks.push_back(console_sink);
        }

        // 创建
        if (conf_->asyncMode) {
            logger_ = std::make_shared<spdlog::async_logger>(
                "async_logger",
                sinks.begin(),
                sinks.end(),
                thread_pool,
                spdlog::async_overflow_policy::block
                );
        } else {
            logger_ = std::make_shared<spdlog::logger>(
                "sync_logger",
                sinks.begin(),
                sinks.end());
        }
        setPattern(conf_->pattern);
        logger_->set_level(convertLevel(conf_->minLogLevel));
    }

    void log(const LogMessage &message) {
        if (!shouldLog(message.level())) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        switch (message.level()) {
            case LogLevel::DEBUG:
                logger_->debug(message.str());
                break;
            case LogLevel::INFO:
                logger_->info(message.str());
                break;
            case LogLevel::WARN:
                logger_->warn(message.str());
                break;
            case LogLevel::ERROR:
                logger_->error(message.str());
                break;
            case LogLevel::FATAL:
                logger_->critical(message.str());
                break;
        }
    }

    void flush() const {
        logger_->flush();
        spdlog::shutdown();
    }

    void setPattern(const std::string &pattern) const {
        logger_->set_pattern(pattern);
    }

    // 便捷日志方法
    LogMessage createLogMessage(LogLevel level, const char* file, int line) {
        return LogMessage(this, level, file, line);
    }

    static void set_config(LoggerConfig *conf);

protected:
    explicit SpdLogger() {
        mutex_.lock();
        if (logger_ == nullptr) {
            initialize();
        }
        mutex_.unlock();
    }

private:
    static SpdLogger *instance;
    const shared_ptr<spdlog::details::thread_pool> thread_pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
    std::shared_ptr<spdlog::logger> logger_;
    static LoggerConfig *conf_;
    std::mutex mutex_;
    static spdlog::level::level_enum convertLevel(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:
                return spdlog::level::debug;
            case LogLevel::INFO:
                return spdlog::level::info;
            case LogLevel::WARN:
                return spdlog::level::warn;
            case LogLevel::ERROR:
                return spdlog::level::err;
            case LogLevel::FATAL:
                return spdlog::level::critical;
            default:
                return spdlog::level::info;
        }
    }
    [[nodiscard]] bool shouldLog(LogLevel level) {
        return level >= conf_->minLogLevel;
    }
};

inline LogMessage::~LogMessage() {
    if (loggerMethod_) {
        loggerMethod_->log(*this);
    }
}

inline void SpdLogger::set_config(LoggerConfig* conf) {
    conf_ = conf;
}

LoggerConfig* SpdLogger::conf_ = nullptr;



#endif //FRAME_LOGGER_H