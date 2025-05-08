#include "../../include/controllers/HealthController.h"
#include "../../include/database/DBConnectionPool.h"
#include "../../include/utils/Logger.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

crow::response HealthController::checkHealth() {
    try {
        // Basic health check
        json response;
        response["status"] = "ok";
        response["timestamp"] = Logger::getCurrentTimestamp();
        response["service"] = "airline-api";

        return crow::response(200, response.dump(4));
    }
    catch (const std::exception& e) {
        Logger::error("Error in health check: " + std::string(e.what()));

        json error;
        error["status"] = "error";
        error["message"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response HealthController::checkDatabaseHealth() {
    try {
        auto& dbPool = DBConnectionPool::getInstance();
        bool dbHealthy = dbPool.checkHealth();

        json response;
        response["status"] = dbHealthy ? "ok" : "error";
        response["timestamp"] = Logger::getCurrentTimestamp();
        response["service"] = "airline-api";
        response["database"] = dbHealthy ? "connected" : "disconnected";

        return crow::response(dbHealthy ? 200 : 503, response.dump(4));
    }
    catch (const std::exception& e) {
        Logger::error("Error in database health check: " + std::string(e.what()));

        json error;
        error["status"] = "error";
        error["message"] = e.what();
        error["database"] = "error";

        return crow::response(500, error.dump(4));
    }
}
