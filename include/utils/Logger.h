#pragma once

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static void init(const std::string& logFile = "airline_api.log");
    static void setLogLevel(LogLevel level);

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);

    static std::string getCurrentTimestamp();

private:
    static void log(LogLevel level, const std::string& message);

    static std::ofstream logFile;
    static std::mutex mutex;
    static LogLevel logLevel;
    static bool initialized;
};
