#ifndef DIAMETER_TESTS_HELPERS_STRING_TO_TIME_H
#define DIAMETER_TESTS_HELPERS_STRING_TO_TIME_H

#include <chrono>
#include <ctime>
#include <sstream>
#include <string>

inline std::chrono::system_clock::time_point string_to_time(const std::string& date)
{
    std::tm tm = {};
    std::stringstream ss(date);
    // 2024-06-06 00:01:11
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse filename time string");
    }
    // Convert timestamp to epoch time assuming UTC
    std::time_t timet = timegm(&tm);
    // Convert timet to std::chrono::time_point<std::chrono::system_clock>
    return std::chrono::system_clock::from_time_t(timet);
}

inline std::string time_to_string(const std::chrono::system_clock::time_point& tp)
{
    std::time_t timet = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = {};
    gmtime_r(&timet, &tm);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        throw std::runtime_error("Failed time to string");
    }
    return ss.str();
}

#endif
