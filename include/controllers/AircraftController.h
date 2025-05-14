#pragma once

#include <crow.h>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../database/DBConnectionPool.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

class AircraftController {
public:
    // Aircraft endpoints
    static crow::response getAircraft(const crow::request& req);
    static crow::response getSingleAircraft(const crow::request& req);
    static crow::response createAircraft(const crow::request& req);
    static crow::response updateAircraft(const crow::request& req);
    static crow::response deleteAircraft(const crow::request& req);
    static crow::response getAircraftFlights(const crow::request& req);
};
