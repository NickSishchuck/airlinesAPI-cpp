#include "../../include/utils/Logger.h"
#include <filesystem>

// Initialize the static instance pointer
Logger* Logger::instance = nullptr;

Logger::Logger() :
    initialized(false),
    currentLevel(LogLevel::INFO),
    showTimestamps(true),
    showSourceInfo(true),
    useColors(true),
    basePath("") {
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger* Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return instance;
}

bool Logger::init() {
    if (initialized) {
        return true;
    }

    // Create logs directory if it doesn't exist
    std::filesystem::create_directory("logs");

    // Create log filename
    logFileName = createLogFileName();

    // Open log file
    logFile.open("logs/" + logFileName + ".log", std::ios::out);

    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: logs/" << logFileName << ".log" << std::endl;
        return false;
    }

    // Try to auto-detect base path from executable location or current working directory
    if (basePath.empty()) {
        // Get current working directory as fallback
        try {
            basePath = std::filesystem::current_path().string();
            // Make sure path ends with a separator
            if (!basePath.empty() && basePath.back() != '/' && basePath.back() != '\\') {
                basePath += '/';
            }
        } catch (const std::exception& e) {
            // If we can't get the current path, leave basePath empty
            std::cerr << "Warning: Could not determine current path: " << e.what() << std::endl;
        }
    }

    initialized = true;

    // Log initial message
    info("Logger initialized: " + logFileName);

    return true;
}

std::string Logger::createLogFileName() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    std::stringstream ss;
    ss << std::setfill('0')
       << std::setw(2) << localTime->tm_mday << "_"
       << std::setw(2) << (localTime->tm_mon + 1) << "_"
       << (localTime->tm_year + 1900) << "_|_"
       << std::setw(2) << localTime->tm_hour << ":"
       << std::setw(2) << localTime->tm_min;

    return ss.str();
}

std::string Logger::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    std::stringstream ss;
    ss << std::setfill('0')
       << "["
       << std::setw(2) << localTime->tm_hour << ":"
       << std::setw(2) << localTime->tm_min << ":"
       << std::setw(2) << localTime->tm_sec
       << "]";

    return ss.str();
}

std::string Logger::getLogLevelString(LogLevel level) {
    if (!useColors) {
        switch (level) {
            case LogLevel::DEBUG:   return "[DEBUG]  ";
            case LogLevel::INFO:    return "[INFO]   ";
            case LogLevel::WARNING: return "[WARNING]";
            case LogLevel::ERROR:   return "[ERROR]  ";
            case LogLevel::FATAL:   return "[FATAL]  ";
            case LogLevel::TODO:    return "[TODO]   ";
            default:                return "[UNKNOWN]";
        }
    } else {
        // Colored format using ANSI codes
        switch (level) {
            case LogLevel::DEBUG:   return "\033[36m[DEBUG]  \033[0m"; // Cyan
            case LogLevel::INFO:    return "\033[32m[INFO]   \033[0m"; // Green
            case LogLevel::WARNING: return "\033[33m[WARNING]\033[0m"; // Yellow
            case LogLevel::ERROR:   return "\033[31m[ERROR]  \033[0m"; // Red
            case LogLevel::FATAL:   return "\033[35m[FATAL]  \033[0m"; // Magenta
            case LogLevel::TODO:    return "\033[34m[TODO]   \033[0m"; // Blue
            default:                return "[UNKNOWN]";
        }
    }
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::enableTimestamps(bool enable) {
    showTimestamps = enable;
}

void Logger::enableSourceInfo(bool enable) {
    showSourceInfo = enable;
}

void Logger::enableColors(bool enable) {
    useColors = enable;
}

void Logger::setBasePath(const std::string& path) {
    basePath = path;

    // Make sure the path ends with a separator
    if (!basePath.empty() && basePath.back() != '/' && basePath.back() != '\\') {
        basePath += '/';
    }
}

// Helper to get the short file path
static std::string getShortFilePath(const std::string& basePath, const std::string& filePath) {
    if (basePath.empty() || filePath.empty()) {
        return filePath;
    }

    // Check if the file path starts with the base path
    if (filePath.find(basePath) == 0) {
        return filePath.substr(basePath.length());
    }

    // Try to find the "src/" part in the path
    size_t srcPos = filePath.find("/src/");
    if (srcPos != std::string::npos) {
        return filePath.substr(srcPos + 1);  // +1 to remove the leading '/'
    }

    // Default to just the filename if we can't find a sensible shortening
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return filePath.substr(lastSlash + 1);
    }

    return filePath;
}

void Logger::logInternal(LogLevel level, const std::string& message, const char* file, int line) {
    if (!initialized && !init()) {
        std::cerr << "Logger not initialized!" << std::endl;
        return;
    }

    // For the console (potentially with colors)
    std::stringstream consoleMessage;
    // For the file (always without colors)
    std::stringstream fileMessage;

    // Add timestamp if enabled
    if (showTimestamps) {
        std::string timestamp = getCurrentTimestamp();
        consoleMessage << (useColors ? "\033[90m" + timestamp + "\033[0m " : timestamp + " "); // Gray color for timestamp
        fileMessage << timestamp << " ";
    }

    // Add log level (color handled inside getLogLevelString)
    consoleMessage << getLogLevelString(level) << " ";
    // For file, always use non-colored version
    switch (level) {
        case LogLevel::DEBUG:   fileMessage << "[DEBUG]  "; break;
        case LogLevel::INFO:    fileMessage << "[INFO]   "; break;
        case LogLevel::WARNING: fileMessage << "[WARNING]"; break;
        case LogLevel::ERROR:   fileMessage << "[ERROR]  "; break;
        case LogLevel::FATAL:   fileMessage << "[FATAL]  "; break;
        case LogLevel::TODO:    fileMessage << "[TODO]   "; break;
        default:                fileMessage << "[UNKNOWN]"; break;
    }
    fileMessage << " ";

    // Add source information if enabled and provided
    if (showSourceInfo && file != nullptr) {
        // Get shorter file path
        std::string shortFilePath = getShortFilePath(basePath, file);
        std::string sourceInfo = "(" + shortFilePath + ":" + std::to_string(line) + ") ";

        consoleMessage << (useColors ? "\033[90m" + sourceInfo + "\033[0m" : sourceInfo); // Gray for source info
        fileMessage << sourceInfo;
    }

    // Add the actual message
    consoleMessage << message;
    fileMessage << message;

    // Write to console with potential colors
    std::cout << consoleMessage.str() << std::endl;

    // Write to file (without color codes)
    if (logFile.is_open()) {
        logFile << fileMessage.str() << std::endl;
        logFile.flush(); // Ensure it's written immediately
    }
}

void Logger::debug(const std::string& message, const char* file, int line) {
    if (static_cast<int>(currentLevel) <= static_cast<int>(LogLevel::DEBUG)) {
        logInternal(LogLevel::DEBUG, message, file, line);
    }
}

void Logger::info(const std::string& message, const char* file, int line) {
    if (static_cast<int>(currentLevel) <= static_cast<int>(LogLevel::INFO)) {
        logInternal(LogLevel::INFO, message, file, line);
    }
}

void Logger::warning(const std::string& message, const char* file, int line) {
    if (static_cast<int>(currentLevel) <= static_cast<int>(LogLevel::WARNING)) {
        logInternal(LogLevel::WARNING, message, file, line);
    }
}

void Logger::error(const std::string& message, const char* file, int line) {
    if (static_cast<int>(currentLevel) <= static_cast<int>(LogLevel::ERROR)) {
        logInternal(LogLevel::ERROR, message, file, line);
    }
}

void Logger::fatal(const std::string& message, const char* file, int line) {
    if (static_cast<int>(currentLevel) <= static_cast<int>(LogLevel::FATAL)) {
        logInternal(LogLevel::FATAL, message, file, line);
    }
}

void Logger::todo(const std::string& message, const char* file, int line) {
    if (static_cast<int>(currentLevel) <= static_cast<int>(LogLevel::TODO)) {
        logInternal(LogLevel::TODO, message, file, line);
    }
}
