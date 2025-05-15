#include "../../include/utils/DateFormat.h"
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

namespace DateFormat {

    std::string formatDate(const std::string& dateStr) {
        // Parse input date string and format to YYYY-MM-DD
        std::tm tm = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&tm, "%Y-%m-%d");

        if (ss.fail()) {
            // Try another format
            ss.clear();
            ss.str(dateStr);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        }

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        return oss.str();
    }

    std::string formatDate(const std::chrono::system_clock::time_point& date) {
        auto t = std::chrono::system_clock::to_time_t(date);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        return oss.str();
    }

    std::string formatTime(const std::string& timeStr) {
        // Parse input time string and format to HH:MM
        std::tm tm = {};
        std::istringstream ss(timeStr);
        ss >> std::get_time(&tm, "%H:%M:%S");

        if (ss.fail()) {
            // Try another format
            ss.clear();
            ss.str(timeStr);
            ss >> std::get_time(&tm, "%H:%M");
        }

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M");
        return oss.str();
    }

    std::string formatTime(const std::chrono::system_clock::time_point& time) {
        auto t = std::chrono::system_clock::to_time_t(time);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M");
        return oss.str();
    }

    std::string formatMySQLDateTime(const std::string& datetimeStr) {
        // Parse input datetime string and format to MySQL datetime
        std::tm tm = {};
        std::istringstream ss(datetimeStr);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

        if (ss.fail()) {
            // Try another format
            ss.clear();
            ss.str(datetimeStr);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        }

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string formatMySQLDateTime(const std::chrono::system_clock::time_point& datetime) {
        auto t = std::chrono::system_clock::to_time_t(datetime);
        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string calculateDuration(const std::string& startStr, const std::string& endStr) {
        // Parse input datetime strings
        std::tm startTm = {}, endTm = {};
        std::istringstream startSs(startStr), endSs(endStr);

        startSs >> std::get_time(&startTm, "%Y-%m-%dT%H:%M:%S");
        endSs >> std::get_time(&endTm, "%Y-%m-%dT%H:%M:%S");

        if (startSs.fail() || endSs.fail()) {
            // Try another format
            startSs.clear();
            endSs.clear();
            startSs.str(startStr);
            endSs.str(endStr);
            startSs >> std::get_time(&startTm, "%Y-%m-%d %H:%M:%S");
            endSs >> std::get_time(&endTm, "%Y-%m-%d %H:%M:%S");
        }

        // Convert to time_t
        std::time_t startTime = std::mktime(&startTm);
        std::time_t endTime = std::mktime(&endTm);

        // Calculate duration in seconds
        double diffSeconds = std::difftime(endTime, startTime);

        // Convert to hours and minutes
        int hours = static_cast<int>(diffSeconds) / 3600;
        int minutes = (static_cast<int>(diffSeconds) % 3600) / 60;

        // Format result
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << hours << ":"
            << std::setfill('0') << std::setw(2) << minutes;

        return oss.str();
    }
}
