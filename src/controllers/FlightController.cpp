#include "../../include/controllers/FlightController.h"
#include <stdexcept>
#include <sstream>

crow::response FlightController::getFlights(const crow::request& req) {
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

        // Prepare query
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
                f.base_price,
                a.model AS aircraft_model,
                a.registration_number,
                c.name AS crew_name
            FROM flights f
            JOIN routes r ON f.route_id = r.route_id
            JOIN aircraft a ON f.aircraft_id = a.aircraft_id
            LEFT JOIN crews c ON a.crew_id = c.crew_id
            ORDER BY f.departure_time
            LIMIT ? OFFSET ?
        )";

        auto stmt = db->prepareStatement(query);
        stmt->setInt(1, limit);
        stmt->setInt(2, offset);

        auto result = db->executeQuery(stmt);

        // Get total count
        auto countStmt = db->prepareStatement("SELECT COUNT(*) as count FROM flights");
        auto countResult = db->executeQuery(countStmt);
        countResult->next();
        int totalCount = countResult->getInt("count");

        // Build response JSON
        json response;
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

            if (!result->isNull("base_price")) {
                flight["base_price"] = result->getDouble("base_price");
            } else {
                flight["base_price"] = nullptr;
            }

            flight["aircraft_model"] = result->getString("aircraft_model");
            flight["registration_number"] = result->getString("registration_number");

            if (!result->isNull("crew_name")) {
                flight["crew_name"] = result->getString("crew_name");
            } else {
                flight["crew_name"] = nullptr;
            }

            flightsArray.push_back(flight);
        }

        response["success"] = true;
        response["count"] = flightsArray.size();
        response["pagination"] = {
            {"page", page},
            {"limit", limit},
            {"totalPages", (int)std::ceil((double)totalCount / limit)},
            {"totalItems", totalCount}
        };
        response["data"] = flightsArray;

        return crow::response(200, response.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getFlights: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getFlights: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}
//TODO
