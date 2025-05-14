#include "../../include/controllers/CrewController.h"
#include <stdexcept>
#include <sstream>

crow::response CrewController::getCrews(const crow::request& req) {
    try {
        // Parse query parameters for pagination
        int page = 1;
        int limit = 10;
        std::string status;

        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }

        if (req.url_params.get("limit")) {
            limit = std::stoi(req.url_params.get("limit"));
        }

        if (req.url_params.get("status")) {
            status = req.url_params.get("status");
        }

        // Calculate offset
        int offset = (page - 1) * limit;

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Build query with status filter if needed
        std::stringstream queryStream;
        queryStream << R"(
            SELECT
                c.crew_id,
                c.name,
                c.status,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_id = c.crew_id) AS member_count,
                (SELECT COUNT(*) FROM aircraft a WHERE a.crew_id = c.crew_id) AS aircraft_count
            FROM crews c
        )";

        if (!status.empty()) {
            queryStream << " WHERE c.status = ?";
        }

        queryStream << R"(
            ORDER BY c.name
            LIMIT ? OFFSET ?
        )";

        std::string query = queryStream.str();
        auto stmt = db->prepareStatement(query);

        int paramIndex = 1;
        if (!status.empty()) {
            stmt->setString(paramIndex++, status);
        }
        stmt->setInt(paramIndex++, limit);
        stmt->setInt(paramIndex, offset);

        auto result = db->executeQuery(stmt);

        // Build count query with the same filter
        std::stringstream countQueryStream;
        countQueryStream << "SELECT COUNT(*) as count FROM crews";

        if (!status.empty()) {
            countQueryStream << " WHERE status = ?";
        }

        auto countStmt = db->prepareStatement(countQueryStream.str());
        if (!status.empty()) {
            countStmt->setString(1, status);
        }

        auto countResult = db->executeQuery(countStmt);
        countResult->next();
        int totalCount = countResult->getInt("count");

        // Build response JSON
        json response;
        json crewsArray = json::array();

        while (result->next()) {
            json crew;
            crew["crew_id"] = result->getInt("crew_id");
            crew["name"] = result->getString("name");
            crew["status"] = result->getString("status");
            crew["member_count"] = result->getInt("member_count");
            crew["aircraft_count"] = result->getInt("aircraft_count");

            crewsArray.push_back(crew);
        }

        response["success"] = true;
        response["count"] = crewsArray.size();
        response["pagination"] = {
            {"page", page},
            {"limit", limit},
            {"totalPages", (int)std::ceil((double)totalCount / limit)},
            {"totalItems", totalCount}
        };
        response["data"] = crewsArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getCrews: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getCrews: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewController::getCrew(const crow::request& req) {
    try {
        int crewId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Get crew details
        std::string query = R"(
            SELECT
                c.crew_id,
                c.name,
                c.status,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_id = c.crew_id) AS member_count,
                (SELECT COUNT(*) FROM aircraft a WHERE a.crew_id = c.crew_id) AS aircraft_count
            FROM crews c
            WHERE c.crew_id = ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, crewId);

        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew not found with id of " + std::to_string(crewId);
            return crow::response(404, error.dump(4));
        }

        // Build response JSON
        json crew;
        crew["crew_id"] = result->getInt("crew_id");
        crew["name"] = result->getString("name");
        crew["status"] = result->getString("status");
        crew["member_count"] = result->getInt("member_count");
        crew["aircraft_count"] = result->getInt("aircraft_count");

        json response;
        response["success"] = true;
        response["data"] = crew;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewController::createCrew(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate required fields
        if (!requestData.contains("name")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide a crew name";
            return crow::response(400, error.dump(4));
        }

        std::string name = requestData["name"];
        std::string status = requestData.contains("status") ? requestData["status"].get<std::string>() : "active";

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Create crew
        std::string query = "INSERT INTO crews (name, status) VALUES (?, ?)";
        auto stmt = db->prepareStatement(query);
        stmt->setString(1, name);
        stmt->setString(2, status);

        db->executeUpdate(stmt);

        // Get the last insert ID
        auto idStmt = db->prepareStatement("SELECT LAST_INSERT_ID()");
        auto idResult = db->executeQuery(idStmt);
        idResult->next();
        int crewId = idResult->getInt(1);

        // Get the created crew
        auto getStmt = db->prepareStatement(R"(
            SELECT
                crew_id,
                name,
                status
            FROM crews
            WHERE crew_id = ?
        )");
        getStmt->setInt(1, crewId);
        auto getResult = db->executeQuery(getStmt);

        getResult->next();

        // Build response JSON
        json crew;
        crew["crew_id"] = getResult->getInt("crew_id");
        crew["name"] = getResult->getString("name");
        crew["status"] = getResult->getString("status");
        crew["member_count"] = 0; // New crew has no members yet
        crew["aircraft_count"] = 0; // New crew has no aircraft yet

        json response;
        response["success"] = true;
        response["data"] = crew;

        return crow::response(201, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in createCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in createCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewController::updateCrew(const crow::request& req) {
    try {
        int crewId = std::stoi(req.url_params.get("id"));

        // Parse request body
        json requestData = json::parse(req.body);

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
        checkStmt->setInt(1, crewId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew not found with id of " + std::to_string(crewId);
            return crow::response(404, error.dump(4));
        }

        // Extract current values
        std::string currentName(checkResult->getString("name").c_str());
        std::string currentStatus(checkResult->getString("status").c_str());

        // Extract update values with fallback to current values
        std::string name = requestData.contains("name") ?
                          requestData["name"].get<std::string>() :
                          currentName;

        std::string status = requestData.contains("status") ?
                            requestData["status"].get<std::string>() :
                            currentStatus;

        // Update crew
        std::string query = "UPDATE crews SET name = ?, status = ? WHERE crew_id = ?";
        auto stmt = db->prepareStatement(query);
        stmt->setString(1, name);
        stmt->setString(2, status);
        stmt->setInt(3, crewId);

        db->executeUpdate(stmt);

        // Get the updated crew
        auto getStmt = db->prepareStatement(R"(
            SELECT
                c.crew_id,
                c.name,
                c.status,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_id = c.crew_id) AS member_count,
                (SELECT COUNT(*) FROM aircraft a WHERE a.crew_id = c.crew_id) AS aircraft_count
            FROM crews c
            WHERE c.crew_id = ?
        )");
        getStmt->setInt(1, crewId);
        auto getResult = db->executeQuery(getStmt);

        getResult->next();

        // Build response JSON
        json crew;
        crew["crew_id"] = getResult->getInt("crew_id");
        crew["name"] = getResult->getString("name");
        crew["status"] = getResult->getString("status");
        crew["member_count"] = getResult->getInt("member_count");
        crew["aircraft_count"] = getResult->getInt("aircraft_count");

        json response;
        response["success"] = true;
        response["data"] = crew;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in updateCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in updateCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewController::deleteCrew(const crow::request& req) {
    try {
        int crewId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
        checkStmt->setInt(1, crewId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew not found with id of " + std::to_string(crewId);
            return crow::response(404, error.dump(4));
        }

        // Check if crew has assigned aircraft
        auto aircraftCheckStmt = db->prepareStatement("SELECT COUNT(*) AS count FROM aircraft WHERE crew_id = ?");
        aircraftCheckStmt->setInt(1, crewId);
        auto aircraftCheckResult = db->executeQuery(aircraftCheckStmt);

        aircraftCheckResult->next();
        if (aircraftCheckResult->getInt("count") > 0) {
            json error;
            error["success"] = false;
            error["error"] = "Cannot delete crew that is assigned to aircraft";
            return crow::response(400, error.dump(4));
        }

        // Start transaction
        db->getConnection()->setAutoCommit(false);

        try {
            // Delete crew assignments first
            auto deleteAssignmentsStmt = db->prepareStatement("DELETE FROM crew_assignments WHERE crew_id = ?");
            deleteAssignmentsStmt->setInt(1, crewId);
            db->executeUpdate(deleteAssignmentsStmt);

            // Delete the crew
            auto deleteCrewStmt = db->prepareStatement("DELETE FROM crews WHERE crew_id = ?");
            deleteCrewStmt->setInt(1, crewId);
            db->executeUpdate(deleteCrewStmt);

            // Commit transaction
            db->getConnection()->commit();

            json response;
            response["success"] = true;
            response["data"] = json::object();

            return crow::response(200, response.dump(4));
        }
        catch (const std::exception& e) {
            // Rollback on error
            db->getConnection()->rollback();
            throw;
        }
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in deleteCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in deleteCrew: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewController::getCrewMembers(const crow::request& req) {
    try {
        int crewId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
        checkStmt->setInt(1, crewId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew not found with id of " + std::to_string(crewId);
            return crow::response(404, error.dump(4));
        }

        // Get crew members
        std::string query = R"(
            SELECT
                cm.crew_member_id,
                cm.first_name,
                cm.last_name,
                cm.role,
                cm.license_number,
                cm.experience_years
            FROM crew_members cm
            JOIN crew_assignments ca ON cm.crew_member_id = ca.crew_member_id
            WHERE ca.crew_id = ?
            ORDER BY cm.role, cm.last_name, cm.first_name
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, crewId);

        auto result = db->executeQuery(stmt);

        // Build response JSON
        json crewMembersArray = json::array();

        while (result->next()) {
            json crewMember;
            crewMember["crew_member_id"] = result->getInt("crew_member_id");
            crewMember["first_name"] = result->getString("first_name");
            crewMember["last_name"] = result->getString("last_name");
            crewMember["role"] = result->getString("role");
            crewMember["license_number"] = result->isNull("license_number") ? nullptr : result->getString("license_number");
            crewMember["experience_years"] = result->getInt("experience_years");

            crewMembersArray.push_back(crewMember);
        }

        json response;
        response["success"] = true;
        response["count"] = crewMembersArray.size();
        response["data"] = crewMembersArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getCrewMembers: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getCrewMembers: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewController::assignCrewMember(const crow::request& req) {
    try {
        int crewId = std::stoi(req.url_params.get("id"));

        // Parse request body
        json requestData = json::parse(req.body);

        // Validate required fields
        if (!requestData.contains("crew_member_id")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide crew_member_id";
            return crow::response(400, error.dump(4));
        }

        int crewMemberId = requestData["crew_member_id"];

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew exists
        auto checkCrewStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
        checkCrewStmt->setInt(1, crewId);
        auto checkCrewResult = db->executeQuery(checkCrewStmt);

        if (!checkCrewResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew not found with id of " + std::to_string(crewId);
            return crow::response(404, error.dump(4));
        }

        // Check if crew member exists
        auto checkMemberStmt = db->prepareStatement("SELECT * FROM crew_members WHERE crew_member_id = ?");
        checkMemberStmt->setInt(1, crewMemberId);
        auto checkMemberResult = db->executeQuery(checkMemberStmt);

        if (!checkMemberResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member not found with id of " + std::to_string(crewMemberId);
            return crow::response(404, error.dump(4));
        }

        // Check if assignment already exists
        auto checkAssignmentStmt = db->prepareStatement(
            "SELECT COUNT(*) AS count FROM crew_assignments WHERE crew_id = ? AND crew_member_id = ?"
        );
        checkAssignmentStmt->setInt(1, crewId);
        checkAssignmentStmt->setInt(2, crewMemberId);
        auto checkAssignmentResult = db->executeQuery(checkAssignmentStmt);

        checkAssignmentResult->next();
        if (checkAssignmentResult->getInt("count") > 0) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member is already assigned to this crew";
            return crow::response(400, error.dump(4));
        }

        // Create assignment
        auto assignStmt = db->prepareStatement(
            "INSERT INTO crew_assignments (crew_id, crew_member_id) VALUES (?, ?)"
        );
        assignStmt->setInt(1, crewId);
        assignStmt->setInt(2, crewMemberId);
        db->executeUpdate(assignStmt);

        // Get updated crew members
        auto membersStmt = db->prepareStatement(R"(
            SELECT
                cm.crew_member_id,
                cm.first_name,
                cm.last_name,
                cm.role,
                cm.license_number,
                cm.experience_years
            FROM crew_members cm
                        JOIN crew_assignments ca ON cm.crew_member_id = ca.crew_member_id
                        WHERE ca.crew_id = ?
                        ORDER BY cm.role, cm.last_name, cm.first_name
                    )");
                    membersStmt->setInt(1, crewId);

                    auto membersResult = db->executeQuery(membersStmt);

                    // Build response JSON
                    json crewMembersArray = json::array();

                    while (membersResult->next()) {
                        json crewMember;
                        crewMember["crew_member_id"] = membersResult->getInt("crew_member_id");
                        crewMember["first_name"] = membersResult->getString("first_name");
                        crewMember["last_name"] = membersResult->getString("last_name");
                        crewMember["role"] = membersResult->getString("role");
                        crewMember["license_number"] = membersResult->isNull("license_number") ? nullptr : membersResult->getString("license_number");
                        crewMember["experience_years"] = membersResult->getInt("experience_years");

                        crewMembersArray.push_back(crewMember);
                    }

                    json response;
                    response["success"] = true;
                    response["count"] = crewMembersArray.size();
                    response["data"] = crewMembersArray;

                    return crow::response(200, response.dump(4));
                }
                catch (const sql::SQLException& e) {
                    LOG_ERROR("SQL error in assignCrewMember: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = "Database error";

                    return crow::response(500, error.dump(4));
                }
                catch (const json::exception& e) {
                    LOG_ERROR("JSON parsing error: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = "Invalid JSON format";

                    return crow::response(400, error.dump(4));
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Error in assignCrewMember: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = e.what();

                    return crow::response(500, error.dump(4));
                }
            }

            crow::response CrewController::removeCrewMember(const crow::request& req) {
                try {
                    int crewId = std::stoi(req.url_params.get("id"));
                    int memberId = std::stoi(req.url_params.get("memberId"));

                    // Get database connection
                    auto db = DBConnectionPool::getInstance().getConnection();

                    // Check if crew exists
                    auto checkCrewStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
                    checkCrewStmt->setInt(1, crewId);
                    auto checkCrewResult = db->executeQuery(checkCrewStmt);

                    if (!checkCrewResult->next()) {
                        json error;
                        error["success"] = false;
                        error["error"] = "Crew not found with id of " + std::to_string(crewId);
                        return crow::response(404, error.dump(4));
                    }

                    // Check if assignment exists
                    auto checkAssignmentStmt = db->prepareStatement(
                        "SELECT COUNT(*) AS count FROM crew_assignments WHERE crew_id = ? AND crew_member_id = ?"
                    );
                    checkAssignmentStmt->setInt(1, crewId);
                    checkAssignmentStmt->setInt(2, memberId);
                    auto checkAssignmentResult = db->executeQuery(checkAssignmentStmt);

                    checkAssignmentResult->next();
                    if (checkAssignmentResult->getInt("count") == 0) {
                        json error;
                        error["success"] = false;
                        error["error"] = "Crew member not found in this crew";
                        return crow::response(404, error.dump(4));
                    }

                    // Check if this crew is assigned to any aircraft
                    auto aircraftStmt = db->prepareStatement("SELECT * FROM aircraft WHERE crew_id = ?");
                    aircraftStmt->setInt(1, crewId);
                    auto aircraftResult = db->executeQuery(aircraftStmt);

                    if (aircraftResult->next()) {
                        // Need to check if removing this member would make the crew invalid
                        // Get the role of the member being removed
                        auto roleStmt = db->prepareStatement(
                            "SELECT role FROM crew_members WHERE crew_member_id = ?"
                        );
                        roleStmt->setInt(1, memberId);
                        auto roleResult = db->executeQuery(roleStmt);

                        if (roleResult->next()) {
                            std::string role = roleResult->getString("role").c_str();

                            // Count members with this role
                            auto countRoleStmt = db->prepareStatement(R"(
                                SELECT COUNT(*) AS count
                                FROM crew_assignments ca
                                JOIN crew_members cm ON ca.crew_member_id = cm.crew_member_id
                                WHERE ca.crew_id = ? AND cm.role = ?
                            )");
                            countRoleStmt->setInt(1, crewId);
                            countRoleStmt->setString(2, role);
                            auto countRoleResult = db->executeQuery(countRoleStmt);

                            countRoleResult->next();
                            int roleCount = countRoleResult->getInt("count");

                            // Check if removing would make the crew invalid
                            if ((role == "captain" && roleCount <= 1) ||
                                (role == "pilot" && roleCount <= 1) ||
                                (role == "flight_attendant" && roleCount <= 2)) {

                                json error;
                                error["success"] = false;
                                error["error"] = "Cannot remove member. Crew would not meet minimum requirements.";
                                return crow::response(400, error.dump(4));
                            }
                        }
                    }

                    // Remove assignment
                    auto removeStmt = db->prepareStatement(
                        "DELETE FROM crew_assignments WHERE crew_id = ? AND crew_member_id = ?"
                    );
                    removeStmt->setInt(1, crewId);
                    removeStmt->setInt(2, memberId);
                    db->executeUpdate(removeStmt);

                    // Get updated crew members
                    auto membersStmt = db->prepareStatement(R"(
                        SELECT
                            cm.crew_member_id,
                            cm.first_name,
                            cm.last_name,
                            cm.role,
                            cm.license_number,
                            cm.experience_years
                        FROM crew_members cm
                        JOIN crew_assignments ca ON cm.crew_member_id = ca.crew_member_id
                        WHERE ca.crew_id = ?
                        ORDER BY cm.role, cm.last_name, cm.first_name
                    )");
                    membersStmt->setInt(1, crewId);

                    auto membersResult = db->executeQuery(membersStmt);

                    // Build response JSON
                    json crewMembersArray = json::array();

                    while (membersResult->next()) {
                        json crewMember;
                        crewMember["crew_member_id"] = membersResult->getInt("crew_member_id");
                        crewMember["first_name"] = membersResult->getString("first_name");
                        crewMember["last_name"] = membersResult->getString("last_name");
                        crewMember["role"] = membersResult->getString("role");
                        crewMember["license_number"] = membersResult->isNull("license_number") ? nullptr : membersResult->getString("license_number");
                        crewMember["experience_years"] = membersResult->getInt("experience_years");

                        crewMembersArray.push_back(crewMember);
                    }

                    json response;
                    response["success"] = true;
                    response["count"] = crewMembersArray.size();
                    response["data"] = crewMembersArray;

                    return crow::response(200, response.dump(4));
                }
                catch (const sql::SQLException& e) {
                    LOG_ERROR("SQL error in removeCrewMember: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = "Database error";

                    return crow::response(500, error.dump(4));
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Error in removeCrewMember: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = e.what();

                    return crow::response(500, error.dump(4));
                }
            }

            crow::response CrewController::getCrewAircraft(const crow::request& req) {
                try {
                    int crewId = std::stoi(req.url_params.get("id"));

                    // Get database connection
                    auto db = DBConnectionPool::getInstance().getConnection();

                    // Check if crew exists
                    auto checkStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
                    checkStmt->setInt(1, crewId);
                    auto checkResult = db->executeQuery(checkStmt);

                    if (!checkResult->next()) {
                        json error;
                        error["success"] = false;
                        error["error"] = "Crew not found with id of " + std::to_string(crewId);
                        return crow::response(404, error.dump(4));
                    }

                    // Get aircraft assigned to this crew
                    std::string query = R"(
                        SELECT
                            a.aircraft_id,
                            a.model,
                            a.registration_number,
                            a.capacity,
                            a.status
                        FROM aircraft a
                        WHERE a.crew_id = ?
                    )";

                    auto stmt = db->prepareStatement(query);
                    stmt->setInt(1, crewId);

                    auto result = db->executeQuery(stmt);

                    // Build response JSON
                    json aircraftArray = json::array();

                    while (result->next()) {
                        json aircraft;
                        aircraft["aircraft_id"] = result->getInt("aircraft_id");
                        aircraft["model"] = result->getString("model");
                        aircraft["registration_number"] = result->getString("registration_number");
                        aircraft["capacity"] = result->getInt("capacity");
                        aircraft["status"] = result->getString("status");

                        aircraftArray.push_back(aircraft);
                    }

                    json response;
                    response["success"] = true;
                    response["count"] = aircraftArray.size();
                    response["data"] = aircraftArray;

                    return crow::response(200, response.dump(4));
                }
                catch (const sql::SQLException& e) {
                    LOG_ERROR("SQL error in getCrewAircraft: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = "Database error";

                    return crow::response(500, error.dump(4));
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Error in getCrewAircraft: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = e.what();

                    return crow::response(500, error.dump(4));
                }
            }

            crow::response CrewController::validateCrew(const crow::request& req) {
                try {
                    int crewId = std::stoi(req.url_params.get("id"));

                    // Get database connection
                    auto db = DBConnectionPool::getInstance().getConnection();

                    // Check if crew exists
                    auto checkStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
                    checkStmt->setInt(1, crewId);
                    auto checkResult = db->executeQuery(checkStmt);

                    if (!checkResult->next()) {
                        json error;
                        error["success"] = false;
                        error["error"] = "Crew not found with id of " + std::to_string(crewId);
                        return crow::response(404, error.dump(4));
                    }

                    // Count by role
                    std::string query = R"(
                        SELECT
                            SUM(CASE WHEN cm.role = 'captain' THEN 1 ELSE 0 END) AS captain_count,
                            SUM(CASE WHEN cm.role = 'pilot' THEN 1 ELSE 0 END) AS pilot_count,
                            SUM(CASE WHEN cm.role = 'flight_attendant' THEN 1 ELSE 0 END) AS attendant_count
                        FROM crew_assignments ca
                        JOIN crew_members cm ON ca.crew_member_id = cm.crew_member_id
                        WHERE ca.crew_id = ?
                    )";

                    auto stmt = db->prepareStatement(query);
                    stmt->setInt(1, crewId);

                    auto result = db->executeQuery(stmt);
                    result->next();

                    int captainCount = result->getInt("captain_count");
                    int pilotCount = result->getInt("pilot_count");
                    int attendantCount = result->getInt("attendant_count");

                    // Validation rules
                    bool isValid = true;
                    json messages = json::array();

                    if (captainCount < 1) {
                        isValid = false;
                        messages.push_back("Crew must have at least one captain");
                    }

                    if (pilotCount < 1) {
                        isValid = false;
                        messages.push_back("Crew must have at least one pilot");
                    }

                    if (attendantCount < 2) {
                        isValid = false;
                        messages.push_back("Crew must have at least two flight attendants");
                    }

                    // Build response JSON
                    json validationResult;
                    validationResult["valid"] = isValid;
                    validationResult["messages"] = messages;
                    validationResult["composition"] = {
                        {"captains", captainCount},
                        {"pilots", pilotCount},
                        {"flight_attendants", attendantCount},
                        {"total", captainCount + pilotCount + attendantCount}
                    };

                    json response;
                    response["success"] = true;
                    response["data"] = validationResult;

                    return crow::response(200, response.dump(4));
                }
                catch (const sql::SQLException& e) {
                    LOG_ERROR("SQL error in validateCrew: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = "Database error";

                    return crow::response(500, error.dump(4));
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Error in validateCrew: " + std::string(e.what()));

                    json error;
                    error["success"] = false;
                    error["error"] = e.what();

                    return crow::response(500, error.dump(4));
                }
            }
