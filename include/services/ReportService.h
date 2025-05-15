#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../database/DBConnectionPool.h"

using json = nlohmann::json;

class ReportService {
public:
    /**
     * Generate detailed revenue report
     * @param startDate Start date (YYYY-MM-DD)
     * @param endDate End date (YYYY-MM-DD)
     * @return Revenue report as JSON
     */
    static json generateRevenueReport(const std::string& startDate, const std::string& endDate);

    /**
     * Generate flight occupancy report
     * @param startDate Start date (YYYY-MM-DD)
     * @param endDate End date (YYYY-MM-DD)
     * @return Occupancy report as JSON
     */
    static json generateOccupancyReport(const std::string& startDate, const std::string& endDate);

    /**
     * Generate route popularity report
     * @param startDate Start date (YYYY-MM-DD)
     * @param endDate End date (YYYY-MM-DD)
     * @return Route popularity report as JSON
     */
    static json generateRoutePopularityReport(const std::string& startDate, const std::string& endDate);
};
