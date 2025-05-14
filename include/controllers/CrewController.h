#pragma once
#include <crow.h>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../database/DBConnectionPool.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

class CrewController {
public:
    // Crew endpoints
    static crow::response getCrews(const crow::request& req);
    static crow::response getCrew(const crow::request& req);
    static crow::response createCrew(const crow::request& req);
    static crow::response updateCrew(const crow::request& req);
    static crow::response deleteCrew(const crow::request& req);
    static crow::response getCrewMembers(const crow::request& req);
    static crow::response assignCrewMember(const crow::request& req);
    static crow::response removeCrewMember(const crow::request& req);
    static crow::response getCrewAircraft(const crow::request& req);
    static crow::response validateCrew(const crow::request& req);
};
