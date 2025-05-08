#include "../../include/utils/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Initialize static members
std::ofstream Logger::logFile;
std::mutex Logger::mutex;
LogLevel Logger::logLevel = LogLevel::INFO;
bool Logger::initialized = false;

void Logger::init(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(mutex);

    if (initialized) {
        return;
    }

    try {
        // Open the log file
        logFile.open(logFilePath, std::ios::out | std::ios::app);

        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file: " << logFilePath << std::endl;
            return;
        }

        initialized = true;

        info("Logger initialized");
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing logger: " << e.what() << std::endl;
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);
    logLevel = level;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < logLevel) {
        return;
    }

    std::string levelStr;
    switch (level) {
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO:  levelStr = "INFO "; break;
        case LogLevel::WARN:  levelStr = "WARN "; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::FATAL: levelStr = "FATAL"; break;
        default:              levelStr = "UNKNOWN";
    }

    std::lock_guard<std::mutex> lock(mutex);

    std::string timestamp = getCurrentTimestamp();
    std::string logMessage = timestamp + " [" + levelStr + "] " + message;

    // Output to console
    std::cout << logMessage << std::endl;

    // Output to file if initialized
    if (initialized && logFile.is_open()) {
        logFile << logMessage << std::endl;
        logFile.flush();
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    return ss.str();
}
