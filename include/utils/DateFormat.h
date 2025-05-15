#pragma once
#include <string>
#include <chrono>

namespace DateFormat {
    /**
     * Format a date to YYYY-MM-DD format
     * @param date The date to format (as a string)
     * @return Formatted date string
     */
    std::string formatDate(const std::string& date);

    /**
     * Format a date to YYYY-MM-DD format
     * @param date The date as a time_point
     * @return Formatted date string
     */
    std::string formatDate(const std::chrono::system_clock::time_point& date);

    /**
     * Format a time to HH:MM format
     * @param time The time to format (as a string)
     * @return Formatted time string
     */
    std::string formatTime(const std::string& time);

    /**
     * Format a time to HH:MM format
     * @param time The time as a time_point
     * @return Formatted time string
     */
    std::string formatTime(const std::chrono::system_clock::time_point& time);

    /**
     * Format a datetime to MySQL datetime format
     * @param datetime The datetime to format (as a string)
     * @return MySQL formatted datetime string
     */
    std::string formatMySQLDateTime(const std::string& datetime);

    /**
     * Format a datetime to MySQL datetime format
     * @param datetime The datetime as a time_point
     * @return MySQL formatted datetime string
     */
    std::string formatMySQLDateTime(const std::chrono::system_clock::time_point& datetime);

    /**
     * Calculate duration between two dates in hours and minutes
     * @param start Start datetime (as a string)
     * @param end End datetime (as a string)
     * @return Duration in HH:MM format
     */
    std::string calculateDuration(const std::string& start, const std::string& end);
}
