#include <iostream>
#include <string>
#include <crow.h>
#include "../include/config/Config.h"
#include "../include/database/DBConnectionPool.h"
#include "../include/controllers/HealthController.h"
#include "../include/utils/Logger.h"

int main() {
    try {
        // Initialize logger
        Logger::init();
        Logger::info("Starting Airline API server...");

        // Load configuration
        Config& config = Config::getInstance();
        if (!config.load("config.json")) {
            Logger::error("Failed to load configuration. Exiting.");
            return 1;
        }

        // Initialize database connection pool
        auto& dbPool = DBConnectionPool::getInstance();
        if (!dbPool.initialize(
                config.getDbHost(),
                config.getDbUser(),
                config.getDbPassword(),
                config.getDbName(),
                config.getDbPort(),
                config.getDbPoolSize())) {
            Logger::error("Failed to initialize database connection pool. Exiting.");
            return 1;
        }

        Logger::info("Database connection pool initialized successfully.");

        // Create and configure Crow application
        crow::SimpleApp app;

        // Add request logging manually since middleware might not be available
        // in this version of Crow

        // Define routes
        CROW_ROUTE(app, "/health")
            .methods("GET"_method)
            ([](const crow::request& req) {
                Logger::info("Request: GET /health");
                auto response = HealthController::checkHealth();
                Logger::info("Response: " + std::to_string(response.code) + " GET /health");
                return response;
            });

        CROW_ROUTE(app, "/health/db")
            .methods("GET"_method)
            ([](const crow::request& req) {
                Logger::info("Request: GET /health/db");
                auto response = HealthController::checkDatabaseHealth();
                Logger::info("Response: " + std::to_string(response.code) + " GET /health/db");
                return response;
            });

        // Start the server
        const int port = config.getPort();
        Logger::info("Server starting on port " + std::to_string(port));
        app.port(port).multithreaded().run();
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
