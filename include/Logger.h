//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_LOGGER_H
#define FRAME_LOGGER_H

#include <string>
#include <sstream>
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>

#define LOG(level) \
    do \
    { \
       SpdLogger* logger = SpdLogger::GetInstance() \
    } while (0)

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

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
        pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v") {}
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
    static SpdLogger *GetInstance(const LoggerConfig* conf) {
        if (instance == nullptr) {
            instance = new SpdLogger(conf);
        }
        return instance;
    }

    void initialize() {
        std::vector<spdlog::sink_ptr> sinks;

        // 文件
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config_->logPath,
            config_->maxFileSize,
            config_->maxFiles
        );
        sinks.push_back(file_sink);
        // 控制台
        if (config_->consoleOutput) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sinks.push_back(console_sink);
        }

        // 创建
        if (config_->asyncMode) {
            spdlog::init_thread_pool(8192, 1);
            logger_ = std::make_unique<spdlog::async_logger>(
                "async_logger",
                sinks.begin(),
                sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::overrun_oldest
                );
        } else {
            logger_ = std::make_unique<spdlog::logger>("sync_logger", sinks.begin(), sinks.end());

        }

        setPattern(config_->pattern);
        logger_->set_level(convertLevel(config_->minLogLevel));
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
    }

    void setPattern(const std::string &pattern) const {
        logger_->set_pattern(pattern);
    }

    // 便捷日志方法
    void createLogMessage(LogLevel level, const char* file, int line) {
        LogMessage(this, level, file, line);
    }
protected:
    explicit SpdLogger(const LoggerConfig* config) {
        config_ = config;
        initialize();
    }

private:
    static SpdLogger *instance;
    std::unique_ptr<spdlog::logger> logger_;
    const LoggerConfig *config_;
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
    [[nodiscard]] bool shouldLog(LogLevel level) const {
        return level >= config_->minLogLevel;
    }
};

inline LogMessage::~LogMessage() {
    if (loggerMethod_) {
        loggerMethod_->log(*this);
    }
}


#endif //FRAME_LOGGER_H