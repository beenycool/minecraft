#include "logger.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    logFile.open("/tmp/minecraft_injectable.log", std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
    }
}

void Logger::log(Level level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream logStream;
    logStream << std::put_time(std::localtime(&time_t), "%F %T") << " ";

    switch (level) {
        case Level::INFO:    logStream << "[INFO]    "; break;
        case Level::DEBUG:   logStream << "[DEBUG]   "; break;
        case Level::WARNING: logStream << "[WARNING] "; break;
        case Level::ERROR:   logStream << "[ERROR]   "; break;
        case Level::SUCCESS: logStream << "[SUCCESS] "; break;
    }

    logStream << message;
    std::string fullMessage = logStream.str();

    // Print to console
    if (level == Level::ERROR) {
        std::cerr << fullMessage << std::endl;
    } else {
        std::cout << fullMessage << std::endl;
    }

    // Write to file
    if (logFile.is_open()) {
        logFile << fullMessage << std::endl;
    }
}

void Logger::flush() {
    Logger& logger = Logger::getInstance();
    if (logger.logFile.is_open()) {
        logger.logFile.flush();
    }
}