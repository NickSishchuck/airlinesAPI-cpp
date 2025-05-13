#include <iostream>
#include <string>
#include <crow.h>
#include "../include/config/Config.h"
#include "../include/database/DBConnectionPool.h"
#include "../include/controllers/HealthController.h"
#include "../include/utils/Logger.h"

int main() {
    try {
        // Print immediate console output to help debug
        std::cout << "Starting application..." << std::endl;
        Logger* logger = Logger::getInstance();

        logger->init();

        // Now we can use the LOG_* macros or the Logger::* methods
        LOG_INFO("Application starting");

        // Load configuration
        LOG_INFO("Loading configuration...");
        Config& config = Config::getInstance();
        if (!config.load("config.json")) {
            LOG_ERROR("Failed to load configuration. Exiting.");
            return 1;
        }
        LOG_INFO("Configuration loaded successfully");

        LOG_INFO("Initializing database connection pool...");
        auto& dbPool = DBConnectionPool::getInstance();

        LOG_DEBUG("About to connect to database at " + config.getDbHost() + ":" + std::to_string(config.getDbPort()));
        LOG_DEBUG("Using database: " + config.getDbName() + ", User: " + config.getDbUser());

        // Try to initialize the database with a timeout and fallback
        bool dbConnected = false;
        try {
            LOG_INFO("Attempting database connection...");
            // Lower pool size to 1 for initial testing to avoid multiple connection attempts
            dbConnected = dbPool.initialize(
                config.getDbHost(),
                config.getDbUser(),
                config.getDbPassword(),
                config.getDbName(),
                config.getDbPort(),
                1); // Start with just 1 connection for faster startup

            LOG_INFO("Database connection attempt completed with result: " +
                    std::string(dbConnected ? "SUCCESS" : "FAILURE"));
        } catch (const std::exception& e) {
            LOG_ERROR("Database connection failed with exception: " + std::string(e.what()));
        }

        // Continue with reduced functionality if database connection fails
        if (!dbConnected) {
            LOG_WARNING("Starting with limited functionality due to database connection failure");
            // You could still continue with non-DB endpoints
        } else {
            LOG_INFO("Database connection pool initialized successfully.");
        }

        // Create and configure Crow application
        LOG_INFO("Creating Crow application...");
        crow::SimpleApp app;

        // Define routes
        LOG_INFO("Defining routes...");
        CROW_ROUTE(app, "/health")
            .methods("GET"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: GET /health");
                auto response = HealthController::checkHealth();
                LOG_INFO("Response: " + std::to_string(response.code) + " GET /health");
                return response;
            });

        CROW_ROUTE(app, "/health/db")
            .methods("GET"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: GET /health/db");
                auto response = HealthController::checkDatabaseHealth();
                LOG_INFO("Response: " + std::to_string(response.code) + " GET /health/db");
                return response;
            });

        // Start the server
        const int port = config.getPort();
        LOG_INFO("Starting server on port " + std::to_string(port) + "...");

        // Use a more basic approach to start the server
        app.port(port);
        app.multithreaded();
        LOG_INFO("Server configured, about to run...");
        app.run();

        // This line will never be reached while the server is running
        LOG_INFO("Server stopped");
    }
    catch (std::exception& e) {
        LOG_FATAL("Error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
