#pragma once

#include <crow.h>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../database/DBConnectionPool.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

class CrewMemberController {
public:
    // Crew Member endpoints
    static crow::response getCrewMembers(const crow::request& req);
    static crow::response getCrewMember(const crow::request& req);
    static crow::response createCrewMember(const crow::request& req);
    static crow::response updateCrewMember(const crow::request& req);
    static crow::response deleteCrewMember(const crow::request& req);
    static crow::response getCrewMemberAssignments(const crow::request& req);
    static crow::response getCrewMemberFlights(const crow::request& req);
    static crow::response searchCrewMembersByLastName(const crow::request& req);
};
