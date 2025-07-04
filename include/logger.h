#pragma once

#include <string>
#include <fstream>

class Logger {
public:
    enum class Level {
        INFO,
        DEBUG,
        WARNING,
        ERROR,
        SUCCESS
    };

    static Logger& getInstance();
    void log(Level level, const std::string& message);
    static void flush();

private:
    Logger();
    std::ofstream logFile;
};

#define LOG_INFO(msg)    Logger::getInstance().log(Logger::Level::INFO, msg)
#define LOG_DEBUG(msg)   Logger::getInstance().log(Logger::Level::DEBUG, msg)
#define LOG_WARN(msg)    Logger::getInstance().log(Logger::Level::WARNING, msg)
#define LOG_ERROR(msg)   Logger::getInstance().log(Logger::Level::ERROR, msg)
#define LOG_SUCCESS(msg) Logger::getInstance().log(Logger::Level::SUCCESS, msg) 