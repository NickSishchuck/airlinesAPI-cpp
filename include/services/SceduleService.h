#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../database/DBConnectionPool.h"

using json = nlohmann::json;

class ScheduleService {
public:
    /**
     * Create recurring flights based on a template
     * @param flightTemplate Base flight details
     * @param startDate Start date (YYYY-MM-DD)
     * @param endDate End date (YYYY-MM-DD)
     * @param daysOfWeek Days to schedule (0=Sunday, 6=Saturday)
     * @return Created flight IDs
     */
    static std::vector<int> createRecurringFlights(
        const json& flightTemplate,
        const std::string& startDate,
        const std::string& endDate,
        const std::vector<int>& daysOfWeek
    );

    /**
     * Check for schedule conflicts
     * @param aircraftId Aircraft ID
     * @param startDate Period start date
     * @param endDate Period end date
     * @return Conflicting flights
     */
    static json checkScheduleConflicts(
        int aircraftId,
        const std::string& startDate,
        const std::string& endDate
    );

    /**
     * Get flights that need to be canceled (no tickets)
     * @param hoursThreshold Hours before departure to check
     * @return Flights to cancel
     */
    static json getEmptyFlightsForCancellation(int hoursThreshold = 24);
};
