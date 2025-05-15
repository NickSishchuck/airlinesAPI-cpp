#pragma once

#include <crow.h>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../database/DBConnectionPool.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

class FlightController {
public:
    // Flight endpoints
    static crow::response getFlights(const crow::request& req);
    static crow::response getFlight(const crow::request& req);
    static crow::response createFlight(const crow::request& req);
    static crow::response updateFlight(const crow::request& req);
    static crow::response deleteFlight(const crow::request& req);
    static crow::response searchFlightsByRouteAndDate(const crow::request& req);
    static crow::response searchFlightsByRoute(const crow::request& req);
    static crow::response generateFlightSchedule(const crow::request& req);
    static crow::response cancelFlight(const crow::request& req);
    static crow::response getFlightPrices(const crow::request& req);
    static crow::response getFlightCrew(const crow::request& req);
    static crow::response getFlightByNumber(const crow::request& req);
};
