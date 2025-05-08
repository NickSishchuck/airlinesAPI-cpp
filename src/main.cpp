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

        // Initialize logger
        Logger::init();
        std::cout << "Logger initialized" << std::endl;
        Logger::info("Starting Airline API server...");

        // Load configuration
        std::cout << "Loading configuration..." << std::endl;
        Config& config = Config::getInstance();
        if (!config.load("config.json")) {
            Logger::error("Failed to load configuration. Exiting.");
            return 1;
        }
        std::cout << "Configuration loaded successfully" << std::endl;

        // Initialize database connection pool
        std::cout << "Initializing database connection pool..." << std::endl;
        auto& dbPool = DBConnectionPool::getInstance();

        std::cout << "About to connect to database at " << config.getDbHost() << ":" << config.getDbPort() << std::endl;

        // Use a timeout for the database connection
        bool dbConnected = false;
        try {
            dbConnected = dbPool.initialize(
                config.getDbHost(),
                config.getDbUser(),
                config.getDbPassword(),
                config.getDbName(),
                config.getDbPort(),
                config.getDbPoolSize());

            std::cout << "Database connection attempt completed" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Database connection failed with exception: " << e.what() << std::endl;
            Logger::error("Database connection failed with exception: " + std::string(e.what()));
        }

        if (!dbConnected) {
            std::cout << "Failed to initialize database connection pool. Exiting." << std::endl;
            Logger::error("Failed to initialize database connection pool. Exiting.");
            return 1;
        }

        std::cout << "Database connection pool initialized successfully." << std::endl;
        Logger::info("Database connection pool initialized successfully.");

        // Create and configure Crow application
        std::cout << "Creating Crow application..." << std::endl;
        crow::SimpleApp app;

        // Define routes
        std::cout << "Defining routes..." << std::endl;
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
        std::cout << "Starting server on port " << port << "..." << std::endl;
        Logger::info("Server starting on port " + std::to_string(port));

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
