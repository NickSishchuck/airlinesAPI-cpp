#include "../../include/controllers/CrewMemberController.h"
#include <stdexcept>
#include <sstream>

crow::response CrewMemberController::getCrewMembers(const crow::request& req) {
    try {
        // Parse query parameters for pagination
        int page = 1;
        int limit = 10;
        std::string role;

        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }

        if (req.url_params.get("limit")) {
            limit = std::stoi(req.url_params.get("limit"));
        }

        if (req.url_params.get("role")) {
            role = req.url_params.get("role");
        }

        // Calculate offset
        int offset = (page - 1) * limit;

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Build query with role filter if needed
        std::stringstream queryStream;
        queryStream << R"(
            SELECT
                cm.crew_member_id,
                cm.first_name,
                cm.last_name,
                cm.role,
                cm.license_number,
                cm.date_of_birth,
                cm.experience_years,
                cm.contact_number,
                cm.email,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_member_id = cm.crew_member_id) AS crew_count
            FROM crew_members cm
        )";

        if (!role.empty()) {
            queryStream << " WHERE cm.role = ?";
        }

        queryStream << R"(
            ORDER BY cm.last_name, cm.first_name
            LIMIT ? OFFSET ?
        )";

        std::string query = queryStream.str();
        auto stmt = db->prepareStatement(query);

        int paramIndex = 1;
        if (!role.empty()) {
            stmt->setString(paramIndex++, role);
        }
        stmt->setInt(paramIndex++, limit);
        stmt->setInt(paramIndex, offset);

        auto result = db->executeQuery(stmt);

        // Build count query with the same filter
        std::stringstream countQueryStream;
        countQueryStream << "SELECT COUNT(*) as count FROM crew_members";

        if (!role.empty()) {
            countQueryStream << " WHERE role = ?";
        }

        auto countStmt = db->prepareStatement(countQueryStream.str());
        if (!role.empty()) {
            countStmt->setString(1, role);
        }

        auto countResult = db->executeQuery(countStmt);
        countResult->next();
        int totalCount = countResult->getInt("count");

        // Build response JSON
        json response;
        json crewMembersArray = json::array();

        while (result->next()) {
            json crewMember;
            crewMember["crew_member_id"] = result->getInt("crew_member_id");
            crewMember["first_name"] = result->getString("first_name");
            crewMember["last_name"] = result->getString("last_name");
            crewMember["role"] = result->getString("role");
            crewMember["license_number"] = result->isNull("license_number") ? nullptr : result->getString("license_number");
            crewMember["date_of_birth"] = result->getString("date_of_birth");
            crewMember["experience_years"] = result->getInt("experience_years");
            crewMember["contact_number"] = result->getString("contact_number");
            crewMember["email"] = result->getString("email");
            crewMember["crew_count"] = result->getInt("crew_count");

            crewMembersArray.push_back(crewMember);
        }

        response["success"] = true;
        response["count"] = crewMembersArray.size();
        response["pagination"] = {
            {"page", page},
            {"limit", limit},
            {"totalPages", (int)std::ceil((double)totalCount / limit)},
            {"totalItems", totalCount}
        };
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

crow::response CrewMemberController::getCrewMember(const crow::request& req) {
    try {
        int crewMemberId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Get crew member details
        std::string query = R"(
            SELECT
                cm.crew_member_id,
                cm.first_name,
                cm.last_name,
                cm.role,
                cm.license_number,
                cm.date_of_birth,
                cm.experience_years,
                cm.contact_number,
                cm.email,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_member_id = cm.crew_member_id) AS crew_count
            FROM crew_members cm
            WHERE cm.crew_member_id = ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, crewMemberId);

        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member not found with id of " + std::to_string(crewMemberId);
            return crow::response(404, error.dump(4));
        }

        // Build response JSON
        json crewMember;
        crewMember["crew_member_id"] = result->getInt("crew_member_id");
        crewMember["first_name"] = result->getString("first_name");
        crewMember["last_name"] = result->getString("last_name");
        crewMember["role"] = result->getString("role");
        crewMember["license_number"] = result->isNull("license_number") ? nullptr : result->getString("license_number");
        crewMember["date_of_birth"] = result->getString("date_of_birth");
        crewMember["experience_years"] = result->getInt("experience_years");
        crewMember["contact_number"] = result->getString("contact_number");
        crewMember["email"] = result->getString("email");
        crewMember["crew_count"] = result->getInt("crew_count");

        json response;
        response["success"] = true;
        response["data"] = crewMember;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getCrewMember: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getCrewMember: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewMemberController::createCrewMember(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate required fields
        if (!requestData.contains("first_name") || !requestData.contains("last_name") ||
            !requestData.contains("role") || !requestData.contains("date_of_birth") ||
            !requestData.contains("experience_years") || !requestData.contains("contact_number") ||
            !requestData.contains("email")) {

            json error;
            error["success"] = false;
            error["error"] = "Missing required fields";
            return crow::response(400, error.dump(4));
        }

        std::string firstName = requestData["first_name"];
        std::string lastName = requestData["last_name"];
        std::string role = requestData["role"];
        std::string licenseNumber = requestData.contains("license_number") ?
                                    requestData["license_number"].get<std::string>() : "";
        std::string dateOfBirth = requestData["date_of_birth"];
        int experienceYears = requestData["experience_years"];
        std::string contactNumber = requestData["contact_number"];
        std::string email = requestData["email"];

        // Validate role
        if (role != "captain" && role != "pilot" && role != "flight_attendant") {
            json error;
            error["success"] = false;
            error["error"] = "Role must be captain, pilot, or flight_attendant";
            return crow::response(400, error.dump(4));
        }

        // Validate license number for captains and pilots
        if ((role == "captain" || role == "pilot") && licenseNumber.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "License number is required for captains and pilots";
            return crow::response(400, error.dump(4));
        }

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if license number already exists (for captain and pilot roles)
        if (!licenseNumber.empty()) {
            auto licenseCheckStmt = db->prepareStatement("SELECT COUNT(*) AS count FROM crew_members WHERE license_number = ?");
            licenseCheckStmt->setString(1, licenseNumber);
            auto licenseCheckResult = db->executeQuery(licenseCheckStmt);

            licenseCheckResult->next();
            if (licenseCheckResult->getInt("count") > 0) {
                json error;
                error["success"] = false;
                error["error"] = "Crew member with this license number already exists";
                return crow::response(409, error.dump(4));
            }
        }

        // Create crew member
        std::string query = R"(
            INSERT INTO crew_members
            (first_name, last_name, role, license_number, date_of_birth, experience_years, contact_number, email)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setString(1, firstName);
        stmt->setString(2, lastName);
        stmt->setString(3, role);
        if (!licenseNumber.empty()) {
            stmt->setString(4, licenseNumber);
        } else {
            stmt->setNull(4, sql::DataType::VARCHAR);
        }
        stmt->setString(5, dateOfBirth);
        stmt->setInt(6, experienceYears);
        stmt->setString(7, contactNumber);
        stmt->setString(8, email);

        db->executeUpdate(stmt);

        // Get the last insert ID
        auto idStmt = db->prepareStatement("SELECT LAST_INSERT_ID()");
        auto idResult = db->executeQuery(idStmt);
        idResult->next();
        int crewMemberId = idResult->getInt(1);

        // Get the created crew member
        auto getStmt = db->prepareStatement(R"(
            SELECT
                crew_member_id,
                first_name,
                last_name,
                role,
                license_number,
                date_of_birth,
                experience_years,
                contact_number,
                email
            FROM crew_members
            WHERE crew_member_id = ?
        )");
        getStmt->setInt(1, crewMemberId);
        auto getResult = db->executeQuery(getStmt);

        getResult->next();

        // Build response JSON
        json crewMember;
        crewMember["crew_member_id"] = getResult->getInt("crew_member_id");
        crewMember["first_name"] = getResult->getString("first_name");
        crewMember["last_name"] = getResult->getString("last_name");
        crewMember["role"] = getResult->getString("role");
        crewMember["license_number"] = getResult->isNull("license_number") ? nullptr : getResult->getString("license_number");
        crewMember["date_of_birth"] = getResult->getString("date_of_birth");
        crewMember["experience_years"] = getResult->getInt("experience_years");
        crewMember["contact_number"] = getResult->getString("contact_number");
        crewMember["email"] = getResult->getString("email");
        crewMember["crew_count"] = 0; // New crew member is not assigned to any crew yet

        json response;
        response["success"] = true;
        response["data"] = crewMember;

        return crow::response(201, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in createCrewMember: " + std::string(e.what()));

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
        LOG_ERROR("Error in createCrewMember: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewMemberController::updateCrewMember(const crow::request& req) {
    try {
        int crewMemberId = std::stoi(req.url_params.get("id"));

        // Parse request body
        json requestData = json::parse(req.body);

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew member exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crew_members WHERE crew_member_id = ?");
        checkStmt->setInt(1, crewMemberId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member not found with id of " + std::to_string(crewMemberId);
            return crow::response(404, error.dump(4));
        }

        // Extract current values
        std::string currentLicenseNumber = checkResult->isNull("license_number") ? "" : checkResult->getString("license_number").c_str();
        std::string currentRole = checkResult->getString("role").c_str();

        // Extract update values with fallback to current values
        std::string firstName = requestData.contains("first_name") ?
                               requestData["first_name"].get<std::string>() :
                               checkResult->getString("first_name").c_str();

        std::string lastName = requestData.contains("last_name") ?
                              requestData["last_name"].get<std::string>() :
                              checkResult->getString("last_name").c_str();

        std::string role = requestData.contains("role") ?
                          requestData["role"].get<std::string>() :
                          currentRole;

        std::string licenseNumber = requestData.contains("license_number") ?
                                   requestData["license_number"].get<std::string>() :
                                   currentLicenseNumber;

        std::string dateOfBirth = requestData.contains("date_of_birth") ?
                                 requestData["date_of_birth"].get<std::string>() :
                                 checkResult->getString("date_of_birth").c_str();

        int experienceYears = requestData.contains("experience_years") ?
                             requestData["experience_years"].get<int>() :
                             checkResult->getInt("experience_years");

        std::string contactNumber = requestData.contains("contact_number") ?
                                   requestData["contact_number"].get<std::string>() :
                                   checkResult->getString("contact_number").c_str();

        std::string email = requestData.contains("email") ?
                           requestData["email"].get<std::string>() :
                           checkResult->getString("email").c_str();

        // Validate role
        if (role != "captain" && role != "pilot" && role != "flight_attendant") {
            json error;
            error["success"] = false;
            error["error"] = "Role must be captain, pilot, or flight_attendant";
            return crow::response(400, error.dump(4));
        }

        // Validate license number for captains and pilots
        if ((role == "captain" || role == "pilot") && licenseNumber.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "License number is required for captains and pilots";
            return crow::response(400, error.dump(4));
        }

        // If changing license number, check if it exists
        if (!licenseNumber.empty() && licenseNumber != currentLicenseNumber) {
            auto licenseCheckStmt = db->prepareStatement(
                "SELECT COUNT(*) AS count FROM crew_members WHERE license_number = ? AND crew_member_id != ?"
            );
            licenseCheckStmt->setString(1, licenseNumber);
            licenseCheckStmt->setInt(2, crewMemberId);
            auto licenseCheckResult = db->executeQuery(licenseCheckStmt);

            licenseCheckResult->next();
            if (licenseCheckResult->getInt("count") > 0) {
                json error;
                error["success"] = false;
                error["error"] = "Crew member with this license number already exists";
                return crow::response(409, error.dump(4));
            }
        }

        // Update crew member
        std::string query = R"(
            UPDATE crew_members
            SET first_name = ?,
                last_name = ?,
                role = ?,
                license_number = ?,
                date_of_birth = ?,
                experience_years = ?,
                contact_number = ?,
                email = ?
            WHERE crew_member_id = ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setString(1, firstName);
        stmt->setString(2, lastName);
        stmt->setString(3, role);

        if (!licenseNumber.empty()) {
            stmt->setString(4, licenseNumber);
        } else {
            stmt->setNull(4, sql::DataType::VARCHAR);
        }

        stmt->setString(5, dateOfBirth);
        stmt->setInt(6, experienceYears);
        stmt->setString(7, contactNumber);
        stmt->setString(8, email);
        stmt->setInt(9, crewMemberId);

        db->executeUpdate(stmt);

        // Get the updated crew member
        auto getStmt = db->prepareStatement(R"(
            SELECT
                cm.crew_member_id,
                cm.first_name,
                cm.last_name,
                cm.role,
                cm.license_number,
                cm.date_of_birth,
                cm.experience_years,
                cm.contact_number,
                cm.email,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_member_id = cm.crew_member_id) AS crew_count
            FROM crew_members cm
            WHERE cm.crew_member_id = ?
        )");
        getStmt->setInt(1, crewMemberId);
        auto getResult = db->executeQuery(getStmt);

        getResult->next();

        // Build response JSON
        json crewMember;
        crewMember["crew_member_id"] = getResult->getInt("crew_member_id");
        crewMember["first_name"] = getResult->getString("first_name");
        crewMember["last_name"] = getResult->getString("last_name");
        crewMember["role"] = getResult->getString("role");
        crewMember["license_number"] = getResult->isNull("license_number") ? nullptr : getResult->getString("license_number");
        crewMember["date_of_birth"] = getResult->getString("date_of_birth");
        crewMember["experience_years"] = getResult->getInt("experience_years");
        crewMember["contact_number"] = getResult->getString("contact_number");
        crewMember["email"] = getResult->getString("email");
        crewMember["crew_count"] = getResult->getInt("crew_count");

        json response;
        response["success"] = true;
        response["data"] = crewMember;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in updateCrewMember: " + std::string(e.what()));

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
        LOG_ERROR("Error in updateCrewMember: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewMemberController::deleteCrewMember(const crow::request& req) {
    try {
        int crewMemberId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew member exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crew_members WHERE crew_member_id = ?");
        checkStmt->setInt(1, crewMemberId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member not found with id of " + std::to_string(crewMemberId);
            return crow::response(404, error.dump(4));
        }

        // Check if this crew member is assigned to any crews
        auto assignmentCheckStmt = db->prepareStatement(
            "SELECT COUNT(*) AS count FROM crew_assignments WHERE crew_member_id = ?"
        );
        assignmentCheckStmt->setInt(1, crewMemberId);
        auto assignmentCheckResult = db->executeQuery(assignmentCheckStmt);

        assignmentCheckResult->next();
        if (assignmentCheckResult->getInt("count") > 0) {
            json error;
            error["success"] = false;
            error["error"] = "Cannot delete crew member who is assigned to a crew";
            return crow::response(400, error.dump(4));
        }

        // Delete crew member
        auto deleteStmt = db->prepareStatement("DELETE FROM crew_members WHERE crew_member_id = ?");
        deleteStmt->setInt(1, crewMemberId);
        db->executeUpdate(deleteStmt);

        json response;
        response["success"] = true;
        response["data"] = json::object();

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in deleteCrewMember: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in deleteCrewMember: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewMemberController::getCrewMemberAssignments(const crow::request& req) {
    try {
        int crewMemberId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew member exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crew_members WHERE crew_member_id = ?");
        checkStmt->setInt(1, crewMemberId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member not found with id of " + std::to_string(crewMemberId);
            return crow::response(404, error.dump(4));
        }

        // Get crews this member is assigned to
        std::string query = R"(
            SELECT
                c.crew_id,
                c.name,
                c.status,
                (SELECT COUNT(*) FROM crew_assignments ca2 WHERE ca2.crew_id = c.crew_id) AS member_count
            FROM crews c
            JOIN crew_assignments ca ON c.crew_id = ca.crew_id
            WHERE ca.crew_member_id = ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, crewMemberId);

        auto result = db->executeQuery(stmt);

        // Build response JSON
        json crewsArray = json::array();

        while (result->next()) {
            json crew;
            crew["crew_id"] = result->getInt("crew_id");
            crew["name"] = result->getString("name");
            crew["status"] = result->getString("status");
            crew["member_count"] = result->getInt("member_count");

            crewsArray.push_back(crew);
        }

        json response;
        response["success"] = true;
        response["count"] = crewsArray.size();
        response["data"] = crewsArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getCrewMemberAssignments: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getCrewMemberAssignments: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewMemberController::getCrewMemberFlights(const crow::request& req) {
    try {
        int crewMemberId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if crew member exists
        auto checkStmt = db->prepareStatement("SELECT * FROM crew_members WHERE crew_member_id = ?");
        checkStmt->setInt(1, crewMemberId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Crew member not found with id of " + std::to_string(crewMemberId);
            return crow::response(404, error.dump(4));
        }

        // Get flights this crew member is on
        std::string query = R"(
            SELECT
                f.flight_id,
                f.flight_number,
                r.origin,
                r.destination,
                f.departure_time,
                f.arrival_time,
                f.status,
                a.model AS aircraft_model,
                a.registration_number
            FROM flights f
            JOIN routes r ON f.route_id = r.route_id
            JOIN aircraft a ON f.aircraft_id = a.aircraft_id
            JOIN crews c ON a.crew_id = c.crew_id
            JOIN crew_assignments ca ON c.crew_id = ca.crew_id
            WHERE ca.crew_member_id = ?
            ORDER BY f.departure_time
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, crewMemberId);

        auto result = db->executeQuery(stmt);

        // Build response JSON
        json flightsArray = json::array();

        while (result->next()) {
            json flight;
            flight["flight_id"] = result->getInt("flight_id");
            flight["flight_number"] = result->getString("flight_number");
            flight["origin"] = result->getString("origin");
            flight["destination"] = result->getString("destination");
            flight["departure_time"] = result->getString("departure_time");
            flight["arrival_time"] = result->getString("arrival_time");
            flight["status"] = result->getString("status");
            flight["aircraft_model"] = result->getString("aircraft_model");
            flight["registration_number"] = result->getString("registration_number");

            flightsArray.push_back(flight);
        }

        json response;
        response["success"] = true;
        response["count"] = flightsArray.size();
        response["data"] = flightsArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getCrewMemberFlights: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getCrewMemberFlights: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response CrewMemberController::searchCrewMembersByLastName(const crow::request& req) {
    try {
        std::string lastName = req.url_params.get("lastName");

        if (lastName.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide a last name to search for";
            return crow::response(400, error.dump(4));
        }

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Search crew members by last name
        std::string query = R"(
            SELECT
                cm.crew_member_id,
                cm.first_name,
                cm.last_name,
                cm.role,
                cm.license_number,
                cm.date_of_birth,
                cm.experience_years,
                cm.contact_number,
                cm.email,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_member_id = cm.crew_member_id) AS crew_count
            FROM crew_members cm
            WHERE cm.last_name = ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setString(1, lastName);

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
            crewMember["date_of_birth"] = result->getString("date_of_birth");
            crewMember["experience_years"] = result->getInt("experience_years");
            crewMember["contact_number"] = result->getString("contact_number");
            crewMember["email"] = result->getString("email");
            crewMember["crew_count"] = result->getInt("crew_count");

            crewMembersArray.push_back(crewMember);
        }

        json response;
        response["success"] = true;
        response["count"] = crewMembersArray.size();
        response["data"] = crewMembersArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in searchCrewMembersByLastName: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in searchCrewMembersByLastName: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}
