#include "../../include/controllers/AircraftController.h"
#include <stdexcept>
#include <sstream>

// Helper function to join strings with a delimiter
std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += strings[i];
    }
    return result;
}

crow::response AircraftController::getAircraft(const crow::request& req) {
    try {
        // Parse query parameters for pagination
        int page = 1;
        int limit = 10;

        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }

        if (req.url_params.get("limit")) {
            limit = std::stoi(req.url_params.get("limit"));
        }

        // Calculate offset
        int offset = (page - 1) * limit;

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Prepare query for aircraft with crew information
        std::string query = R"(
            SELECT
                a.aircraft_id,
                a.model,
                a.registration_number,
                a.capacity,
                a.manufacturing_year,
                a.crew_id,
                a.status,
                c.name AS crew_name,
                (SELECT COUNT(*) FROM crew_assignments ca WHERE ca.crew_id = c.crew_id) AS crew_size
            FROM aircraft a
            LEFT JOIN crews c ON a.crew_id = c.crew_id
            ORDER BY a.registration_number
            LIMIT ? OFFSET ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, limit);
        stmt->setInt(2, offset);

        auto result = db->executeQuery(stmt);

        // Get total count
        auto countStmt = db->prepareStatement("SELECT COUNT(*) as count FROM aircraft");
        auto countResult = db->executeQuery(countStmt);
        countResult->next();
        int totalCount = countResult->getInt("count");

        // Build response JSON
        json response;
        json aircraftArray = json::array();

        while (result->next()) {
            json aircraft;
            aircraft["aircraft_id"] = result->getInt("aircraft_id");
            aircraft["model"] = result->getString("model");
            aircraft["registration_number"] = result->getString("registration_number");
            aircraft["capacity"] = result->getInt("capacity");
            aircraft["manufacturing_year"] = result->getInt("manufacturing_year");

            if (!result->isNull("crew_id")) {
                aircraft["crew_id"] = result->getInt("crew_id");
                aircraft["crew_name"] = result->getString("crew_name");
                aircraft["crew_size"] = result->getInt("crew_size");
            } else {
                aircraft["crew_id"] = nullptr;
                aircraft["crew_name"] = nullptr;
                aircraft["crew_size"] = 0;
            }

            aircraft["status"] = result->getString("status");

            aircraftArray.push_back(aircraft);
        }

        response["success"] = true;
        response["count"] = aircraftArray.size();
        response["pagination"] = {
            {"page", page},
            {"limit", limit},
            {"totalPages", (int)std::ceil((double)totalCount / limit)},
            {"totalItems", totalCount}
        };
        response["data"] = aircraftArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AircraftController::getSingleAircraft(const crow::request& req) {
    try {
        int aircraftId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Prepare query for aircraft with crew information
        std::string query = R"(
            SELECT
                a.aircraft_id,
                a.model,
                a.registration_number,
                a.capacity,
                a.manufacturing_year,
                a.crew_id,
                a.status,
                c.name AS crew_name,
                c.status AS crew_status
            FROM aircraft a
            LEFT JOIN crews c ON a.crew_id = c.crew_id
            WHERE a.aircraft_id = ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, aircraftId);

        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Aircraft not found with id of " + std::to_string(aircraftId);
            return crow::response(404, error.dump(4));
        }

        // Build response JSON
        json aircraft;
        aircraft["aircraft_id"] = result->getInt("aircraft_id");
        aircraft["model"] = result->getString("model");
        aircraft["registration_number"] = result->getString("registration_number");
        aircraft["capacity"] = result->getInt("capacity");
        aircraft["manufacturing_year"] = result->getInt("manufacturing_year");

        if (!result->isNull("crew_id")) {
            aircraft["crew_id"] = result->getInt("crew_id");
            aircraft["crew_name"] = result->getString("crew_name");
            aircraft["crew_status"] = result->getString("crew_status");
        } else {
            aircraft["crew_id"] = nullptr;
            aircraft["crew_name"] = nullptr;
            aircraft["crew_status"] = nullptr;
        }

        aircraft["status"] = result->getString("status");

        json response;
        response["success"] = true;
        response["data"] = aircraft;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getSingleAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getSingleAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AircraftController::createAircraft(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate required fields
        if (!requestData.contains("model") || !requestData.contains("registration_number") || !requestData.contains("capacity") || !requestData.contains("manufacturing_year")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide model, registration_number, capacity and manufacturing_year";
            return crow::response(400, error.dump(4));
        }

        std::string model = requestData["model"];
        std::string registrationNumber = requestData["registration_number"];
        int capacity = requestData["capacity"];
        int manufacturingYear = requestData["manufacturing_year"];
        int crewId = requestData.contains("crew_id") ? requestData["crew_id"].get<int>() : 0;
        std::string status = requestData.contains("status") ? requestData["status"].get<std::string>() : "active";

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if registration number already exists
        auto checkStmt = db->prepareStatement("SELECT COUNT(*) AS count FROM aircraft WHERE registration_number = ?");
        checkStmt->setString(1, registrationNumber);
        auto checkResult = db->executeQuery(checkStmt);

        checkResult->next();
        if (checkResult->getInt("count") > 0) {
            json error;
            error["success"] = false;
            error["error"] = "Aircraft with this registration number already exists";
            return crow::response(409, error.dump(4));
        }

        // If crew_id is provided, validate crew
        if (crewId > 0) {
            auto crewStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
            crewStmt->setInt(1, crewId);
            auto crewResult = db->executeQuery(crewStmt);

            if (!crewResult->next()) {
                json error;
                error["success"] = false;
                error["error"] = "Crew not found with id of " + std::to_string(crewId);
                return crow::response(404, error.dump(4));
            }

            // Validate crew composition
            // Check if crew has at least 1 captain, 1 pilot, and 2 flight attendants
            auto validateStmt = db->prepareStatement(R"(
                SELECT
                    SUM(CASE WHEN cm.role = 'captain' THEN 1 ELSE 0 END) AS captain_count,
                    SUM(CASE WHEN cm.role = 'pilot' THEN 1 ELSE 0 END) AS pilot_count,
                    SUM(CASE WHEN cm.role = 'flight_attendant' THEN 1 ELSE 0 END) AS attendant_count
                FROM crew_assignments ca
                JOIN crew_members cm ON ca.crew_member_id = cm.crew_member_id
                WHERE ca.crew_id = ?
            )");
            validateStmt->setInt(1, crewId);
            auto validateResult = db->executeQuery(validateStmt);

            validateResult->next();
            int captainCount = validateResult->getInt("captain_count");
            int pilotCount = validateResult->getInt("pilot_count");
            int attendantCount = validateResult->getInt("attendant_count");

            std::vector<std::string> validationMessages;
            bool isValid = true;

            if (captainCount < 1) {
                validationMessages.push_back("Crew must have at least one captain");
                isValid = false;
            }

            if (pilotCount < 1) {
                validationMessages.push_back("Crew must have at least one pilot");
                isValid = false;
            }

            if (attendantCount < 2) {
                validationMessages.push_back("Crew must have at least two flight attendants");
                isValid = false;
            }

            if (!isValid) {
                json error;
                error["success"] = false;
                error["error"] = "Invalid crew composition: " + join(validationMessages, ", ");
                return crow::response(400, error.dump(4));
            }
        }

        // Insert aircraft
        std::string insertQuery;
        std::unique_ptr<sql::PreparedStatement> insertStmt;

        if (crewId > 0) {
            insertQuery = R"(
                INSERT INTO aircraft
                (model, registration_number, capacity, manufacturing_year, crew_id, status)
                VALUES (?, ?, ?, ?, ?, ?)
            )";
            insertStmt = db->prepareStatement(insertQuery);
            insertStmt->setString(1, model);
            insertStmt->setString(2, registrationNumber);
            insertStmt->setInt(3, capacity);
            insertStmt->setInt(4, manufacturingYear);
            insertStmt->setInt(5, crewId);
            insertStmt->setString(6, status);
        } else {
            insertQuery = R"(
                INSERT INTO aircraft
                (model, registration_number, capacity, manufacturing_year, status)
                VALUES (?, ?, ?, ?, ?)
            )";
            insertStmt = db->prepareStatement(insertQuery);
            insertStmt->setString(1, model);
            insertStmt->setString(2, registrationNumber);
            insertStmt->setInt(3, capacity);
            insertStmt->setInt(4, manufacturingYear);
            insertStmt->setString(5, status);
        }

        db->executeUpdate(insertStmt);

        // Get the last insert ID
        auto idStmt = db->prepareStatement("SELECT LAST_INSERT_ID()");
        auto idResult = db->executeQuery(idStmt);
        idResult->next();
        int aircraftId = idResult->getInt(1);

        // Get the created aircraft
        auto getStmt = db->prepareStatement(R"(
            SELECT
                a.aircraft_id,
                a.model,
                a.registration_number,
                a.capacity,
                a.manufacturing_year,
                a.crew_id,
                a.status,
                c.name AS crew_name,
                c.status AS crew_status
            FROM aircraft a
            LEFT JOIN crews c ON a.crew_id = c.crew_id
            WHERE a.aircraft_id = ?
        )");
        getStmt->setInt(1, aircraftId);
        auto getResult = db->executeQuery(getStmt);

        getResult->next();

        // Build response JSON
        json aircraft;
        aircraft["aircraft_id"] = getResult->getInt("aircraft_id");
        aircraft["model"] = getResult->getString("model");
        aircraft["registration_number"] = getResult->getString("registration_number");
        aircraft["capacity"] = getResult->getInt("capacity");
        aircraft["manufacturing_year"] = getResult->getInt("manufacturing_year");

        if (!getResult->isNull("crew_id")) {
            aircraft["crew_id"] = getResult->getInt("crew_id");
            aircraft["crew_name"] = getResult->getString("crew_name");
            aircraft["crew_status"] = getResult->getString("crew_status");
        } else {
            aircraft["crew_id"] = nullptr;
            aircraft["crew_name"] = nullptr;
            aircraft["crew_status"] = nullptr;
        }

        aircraft["status"] = getResult->getString("status");

        json response;
        response["success"] = true;
        response["data"] = aircraft;

        return crow::response(201, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in createAircraft: " + std::string(e.what()));

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
        LOG_ERROR("Error in createAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}


crow::response AircraftController::updateAircraft(const crow::request& req) {
    try {
        int aircraftId = std::stoi(req.url_params.get("id"));

        // Parse request body
        json requestData = json::parse(req.body);

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if aircraft exists
        auto checkStmt = db->prepareStatement("SELECT * FROM aircraft WHERE aircraft_id = ?");
        checkStmt->setInt(1, aircraftId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Aircraft not found with id of " + std::to_string(aircraftId);
            return crow::response(404, error.dump(4));
        }

        // Extract current values
        std::string currentRegistration = static_cast<std::string>(checkResult->getString("registration_number"));
        int currentCrewId = checkResult->isNull("crew_id") ? 0 : checkResult->getInt("crew_id");

        // Extract update values
        std::string model = requestData.contains("model") ?
                          requestData["model"].get<std::string>() :
                          std::string(checkResult->getString("model").c_str());
        std::string registrationNumber = requestData.contains("registration_number") ? requestData["registration_number"].get<std::string>() : currentRegistration;
        int capacity = requestData.contains("capacity") ? requestData["capacity"].get<int>() : checkResult->getInt("capacity");
        int manufacturingYear = requestData.contains("manufacturing_year") ? requestData["manufacturing_year"].get<int>() : checkResult->getInt("manufacturing_year");
        int crewId = requestData.contains("crew_id") ? requestData["crew_id"].get<int>() : currentCrewId;

        std::string status = requestData.contains("status") ?
                           requestData["status"].get<std::string>() :
                           std::string(checkResult->getString("status").c_str());

        // If changing registration, check if it exists
        if (registrationNumber != currentRegistration) {
            auto regCheckStmt = db->prepareStatement("SELECT COUNT(*) AS count FROM aircraft WHERE registration_number = ? AND aircraft_id != ?");
            regCheckStmt->setString(1, registrationNumber);
            regCheckStmt->setInt(2, aircraftId);
            auto regCheckResult = db->executeQuery(regCheckStmt);

            regCheckResult->next();
            if (regCheckResult->getInt("count") > 0) {
                json error;
                error["success"] = false;
                error["error"] = "Aircraft with this registration number already exists";
                return crow::response(409, error.dump(4));
            }
        }

        // If updating crew_id, validate crew
        if (crewId != currentCrewId && crewId > 0) {
            auto crewStmt = db->prepareStatement("SELECT * FROM crews WHERE crew_id = ?");
            crewStmt->setInt(1, crewId);
            auto crewResult = db->executeQuery(crewStmt);

            if (!crewResult->next()) {
                json error;
                error["success"] = false;
                error["error"] = "Crew not found with id of " + std::to_string(crewId);
                return crow::response(404, error.dump(4));
            }

            // Validate crew composition
            auto validateStmt = db->prepareStatement(R"(
                SELECT
                    SUM(CASE WHEN cm.role = 'captain' THEN 1 ELSE 0 END) AS captain_count,
                    SUM(CASE WHEN cm.role = 'pilot' THEN 1 ELSE 0 END) AS pilot_count,
                    SUM(CASE WHEN cm.role = 'flight_attendant' THEN 1 ELSE 0 END) AS attendant_count
                FROM crew_assignments ca
                JOIN crew_members cm ON ca.crew_member_id = cm.crew_member_id
                WHERE ca.crew_id = ?
            )");
            validateStmt->setInt(1, crewId);
            auto validateResult = db->executeQuery(validateStmt);

            validateResult->next();
            int captainCount = validateResult->getInt("captain_count");
            int pilotCount = validateResult->getInt("pilot_count");
            int attendantCount = validateResult->getInt("attendant_count");

            std::vector<std::string> validationMessages;
            bool isValid = true;

            if (captainCount < 1) {
                validationMessages.push_back("Crew must have at least one captain");
                isValid = false;
            }

            if (pilotCount < 1) {
                validationMessages.push_back("Crew must have at least one pilot");
                isValid = false;
            }

            if (attendantCount < 2) {
                validationMessages.push_back("Crew must have at least two flight attendants");
                isValid = false;
            }

            if (!isValid) {
                json error;
                error["success"] = false;
                error["error"] = "Invalid crew composition: " + join(validationMessages, ", ");
                return crow::response(400, error.dump(4));
            }
        }

        // Update aircraft
        std::string updateQuery;
        std::unique_ptr<sql::PreparedStatement> updateStmt;

        if (crewId > 0) {
            updateQuery = R"(
                UPDATE aircraft
                SET model = ?, registration_number = ?, capacity = ?,
                    manufacturing_year = ?, crew_id = ?, status = ?
                WHERE aircraft_id = ?
            )";
            updateStmt = db->prepareStatement(updateQuery);
            updateStmt->setString(1, model);
            updateStmt->setString(2, registrationNumber);
            updateStmt->setInt(3, capacity);
            updateStmt->setInt(4, manufacturingYear);
            updateStmt->setInt(5, crewId);
            updateStmt->setString(6, status);
            updateStmt->setInt(7, aircraftId);
        } else {
            updateQuery = R"(
                UPDATE aircraft
                SET model = ?, registration_number = ?, capacity = ?,
                    manufacturing_year = ?, crew_id = NULL, status = ?
                WHERE aircraft_id = ?
            )";
            updateStmt = db->prepareStatement(updateQuery);
            updateStmt->setString(1, model);
            updateStmt->setString(2, registrationNumber);
            updateStmt->setInt(3, capacity);
            updateStmt->setInt(4, manufacturingYear);
            updateStmt->setString(5, status);
            updateStmt->setInt(6, aircraftId);
        }

        db->executeUpdate(updateStmt);

        // Get the updated aircraft
        auto getStmt = db->prepareStatement(R"(
            SELECT
                a.aircraft_id,
                a.model,
                a.registration_number,
                a.capacity,
                a.manufacturing_year,
                a.crew_id,
                a.status,
                c.name AS crew_name,
                c.status AS crew_status
            FROM aircraft a
            LEFT JOIN crews c ON a.crew_id = c.crew_id
            WHERE a.aircraft_id = ?
        )");
        getStmt->setInt(1, aircraftId);
        auto getResult = db->executeQuery(getStmt);

        getResult->next();

        // Build response JSON
        json aircraft;
        aircraft["aircraft_id"] = getResult->getInt("aircraft_id");
        aircraft["model"] = getResult->getString("model");
        aircraft["registration_number"] = getResult->getString("registration_number");
        aircraft["capacity"] = getResult->getInt("capacity");
        aircraft["manufacturing_year"] = getResult->getInt("manufacturing_year");

        if (!getResult->isNull("crew_id")) {
            aircraft["crew_id"] = getResult->getInt("crew_id");
            aircraft["crew_name"] = getResult->getString("crew_name");
            aircraft["crew_status"] = getResult->getString("crew_status");
        } else {
            aircraft["crew_id"] = nullptr;
            aircraft["crew_name"] = nullptr;
            aircraft["crew_status"] = nullptr;
        }

        aircraft["status"] = getResult->getString("status");

        json response;
        response["success"] = true;
        response["data"] = aircraft;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in updateAircraft: " + std::string(e.what()));

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
        LOG_ERROR("Error in updateAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AircraftController::deleteAircraft(const crow::request& req) {
    try {
        int aircraftId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if aircraft exists
        auto checkStmt = db->prepareStatement("SELECT * FROM aircraft WHERE aircraft_id = ?");
        checkStmt->setInt(1, aircraftId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Aircraft not found with id of " + std::to_string(aircraftId);
            return crow::response(404, error.dump(4));
        }

        // Check if aircraft has flights
        auto flightCheckStmt = db->prepareStatement(R"(
            SELECT COUNT(*) AS count
            FROM flights
            WHERE aircraft_id = ? AND status NOT IN ('canceled', 'arrived')
        )");
        flightCheckStmt->setInt(1, aircraftId);
        auto flightCheckResult = db->executeQuery(flightCheckStmt);

        flightCheckResult->next();
        if (flightCheckResult->getInt("count") > 0) {
            json error;
            error["success"] = false;
            error["error"] = "Cannot delete aircraft with associated flights";
            return crow::response(400, error.dump(4));
        }

        // Delete aircraft
        auto deleteStmt = db->prepareStatement("DELETE FROM aircraft WHERE aircraft_id = ?");
        deleteStmt->setInt(1, aircraftId);
        db->executeUpdate(deleteStmt);

        json response;
        response["success"] = true;
        response["data"] = json::object();

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in deleteAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in deleteAircraft: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AircraftController::getAircraftFlights(const crow::request& req) {
    try {
        int aircraftId = std::stoi(req.url_params.get("id"));

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if aircraft exists
        auto checkStmt = db->prepareStatement("SELECT * FROM aircraft WHERE aircraft_id = ?");
        checkStmt->setInt(1, aircraftId);
        auto checkResult = db->executeQuery(checkStmt);

        if (!checkResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Aircraft not found with id of " + std::to_string(aircraftId);
            return crow::response(404, error.dump(4));
        }

        // Get active flights only parameter (optional)
        bool activeOnly = false;
        if (req.url_params.get("activeOnly")) {
            std::string activeOnlyParam = req.url_params.get("activeOnly");
            activeOnly = (activeOnlyParam == "true" || activeOnlyParam == "1");
        }

        // Prepare query to get flights
        std::string query = R"(
            SELECT
                f.flight_id,
                f.flight_number,
                r.origin,
                r.destination,
                f.departure_time,
                f.arrival_time,
                f.status,
                f.gate,
                f.base_price
            FROM flights f
            JOIN routes r ON f.route_id = r.route_id
            WHERE f.aircraft_id = ?
        )";

        if (activeOnly) {
            query += " AND f.status NOT IN ('canceled', 'arrived')";
        }

        query += " ORDER BY f.departure_time";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, aircraftId);

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
            flight["gate"] = result->isNull("gate") ? nullptr : result->getString("gate");
            if (result->isNull("base_price")) {
                flight["base_price"] = nullptr;
            } else {
                flight["base_price"] = result->getDouble("base_price");
            }

            flightsArray.push_back(flight);
        }

        json response;
        response["success"] = true;
        response["count"] = flightsArray.size();
        response["data"] = flightsArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getAircraftFlights: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getAircraftFlights: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}
