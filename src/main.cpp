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

        // Load configuration
        std::cout << "Loading configuration..." << std::endl;
        Config& config = Config::getInstance();
        if (!config.load("config.json")) {
            Logger::error("Failed to load configuration. Exiting.");
            return 1;
        }
        std::cout << "Configuration loaded successfully" << std::endl;
    std::cout << "Initializing database connection pool..." << std::endl;
    auto& dbPool = DBConnectionPool::getInstance();

    std::cout << "About to connect to database at " << config.getDbHost() << ":" << config.getDbPort() << std::endl;
    std::cout << "Using database: " << config.getDbName() << ", User: " << config.getDbUser() << std::endl;

    // Try to initialize the database with a timeout and fallback
    bool dbConnected = false;
    try {
        std::cout << "Attempting database connection..." << std::endl;
        // Lower pool size to 1 for initial testing to avoid multiple connection attempts
        dbConnected = dbPool.initialize(
            config.getDbHost(),
            config.getDbUser(),
            config.getDbPassword(),
            config.getDbName(),
            config.getDbPort(),
            1); // Start with just 1 connection for faster startup

        std::cout << "Database connection attempt completed with result: "
                  << (dbConnected ? "SUCCESS" : "FAILURE") << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Database connection failed with exception: " << e.what() << std::endl;
    }

    // Continue with reduced functionality if database connection fails
    if (!dbConnected) {
        std::cout << "NOTICE: Starting with limited functionality due to database connection failure" << std::endl;
        // You could still continue with non-DB endpoints
    } else {
        std::cout << "Database connection pool initialized successfully." << std::endl;
    }

        // Create and configure Crow application
        std::cout << "Creating Crow application..." << std::endl;
        crow::SimpleApp app;

        // Define routes
        std::cout << "Defining routes..." << std::endl;
        CROW_ROUTE(app, "/health")
            .methods("GET"_method)
            ([](const crow::request& req) {
                std::cout << (" Request: GET /health");
                auto response = HealthController::checkHealth();
                std::cout << (" Response: " + std::to_string(response.code) + " GET /health");
                return response;
            });

        CROW_ROUTE(app, "/health/db")
            .methods("GET"_method)
            ([](const crow::request& req) {
               std::cout << (" Request: GET /health/db");
                auto response = HealthController::checkDatabaseHealth();
                std::cout << (" Response: " + std::to_string(response.code) + " GET /health/db");
                return response;
            });

        // Start the server
        const int port = config.getPort();
        std::cout << "Starting server on port " << port << "..." << std::endl;

        // Use a more basic approach to start the server
        app.port(port);
        app.multithreaded();
        std::cout << "Server configured, about to run..." << std::endl;
        app.run();

        // This line will never be reached while the server is running
        std::cout << "Server stopped" << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
