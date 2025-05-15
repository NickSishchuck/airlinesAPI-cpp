#include <iostream>
#include <string>
#include <crow.h>
#include <crow/middlewares/cors.h>
#include "../include/config/Config.h"
#include "../include/database/DBConnectionPool.h"
#include "../include/controllers/HealthController.h"
#include "../include/controllers/AuthController.h"
#include "../include/middleware/AuthMiddleware.h"
#include "../include/controllers/AircraftController.h"
#include "../include/controllers/CrewMemberController.h"
#include "../include/controllers/CrewController.h"
#include "../include/controllers/FlightController.h"
#include "../include/utils/Logger.h"

int main() {
    try {
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

        // Initialize JWT utils with configuration
        JWTUtils::getInstance().setSecret(config.getJwtSecret());
        JWTUtils::getInstance().setExpiresIn(config.getJwtExpiresIn());
        LOG_INFO("JWT utils initialized");

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

        // Create and configure Crow application with middlewares
        LOG_INFO("Creating Crow application...");
        crow::App<crow::CORSHandler, AuthMiddleware> app;

        // Configure CORS
        auto& cors = app.get_middleware<crow::CORSHandler>();
        cors
            .global()
            .headers("Authorization", "Content-Type")
            .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "PATCH"_method)
            .origin("*");
        LOG_TODO("Specify allowed origin");

        // Health check routes
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

        // Auth routes

        CROW_ROUTE(app, "/api/auth/register")
            .methods("POST"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: POST /api/auth/register");
                auto response = AuthController::registerEmail(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/auth/register");
                return response;
            });

        CROW_ROUTE(app, "/api/auth/register/phone")
            .methods("POST"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: POST /api/auth/register/phone");
                auto response = AuthController::registerPhone(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/auth/register/phone");
                return response;
            });

        CROW_ROUTE(app, "/api/auth/login")
            .methods("POST"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: POST /api/auth/login");
                auto response = AuthController::login(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/auth/login");
                return response;
            });

        // Login with phone
        CROW_ROUTE(app, "/api/auth/login/phone")
            .methods("POST"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: POST /api/auth/login/phone");
                auto response = AuthController::loginPhone(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/auth/login/phone");
                return response;
            });

        // Get current user - protected route
        CROW_ROUTE(app, "/api/auth/me")
            .methods("GET"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: GET /api/auth/me");

                // Check authentication
                if (!is_authenticated(req)) {
                    return auth_error(401, "Not authorized to access this route");
                }

                // Check authorization
                if (!has_role(req, {"admin", "worker", "user"})) {
                    return auth_error(403, "User role is not authorized to access this route");
                }

                auto response = AuthController::getMe(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/auth/me");
                return response;
            });

        // Update password - protected route
        CROW_ROUTE(app, "/api/auth/updatepassword")
            .methods("PUT"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: PUT /api/auth/updatepassword");

                // Check authentication
                if (!is_authenticated(req)) {
                    return auth_error(401, "Not authorized to access this route");
                }

                // Check authorization
                if (!has_role(req, {"admin", "worker", "user"})) {
                    return auth_error(403, "User role is not authorized to access this route");
                }

                auto response = AuthController::updatePassword(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " PUT /api/auth/updatepassword");
                return response;
            });

        // Logout - protected route
        CROW_ROUTE(app, "/api/auth/logout")
            .methods("GET"_method)
            ([](const crow::request& req) {
                LOG_INFO("Request: GET /api/auth/logout");

                // Check authentication
                if (!is_authenticated(req)) {
                    return auth_error(401, "Not authorized to access this route");
                }

                auto response = AuthController::logout(req);
                LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/auth/logout");
                return response;
            });


            // Aircraft routes
            CROW_ROUTE(app, "/api/aircraft")
                .methods("GET"_method)
                ([](const crow::request& req) {
                    LOG_INFO("Request: GET /api/aircraft");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access these aircraft");
                    }

                    auto response = AircraftController::getAircraft(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/aircraft");
                    return response;
                });

            CROW_ROUTE(app, "/api/aircraft")
                .methods("POST"_method)
                ([](const crow::request& req) {
                    LOG_INFO("Request: POST /api/aircraft");

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to create aircraft");
                    }

                    auto response = AircraftController::createAircraft(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/aircraft");
                    return response;
                });

            CROW_ROUTE(app, "/api/aircraft/<int>")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/aircraft/" + std::to_string(id));

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access this aircraft");
                    }

                    auto response = AircraftController::getSingleAircraft(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/aircraft/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/aircraft/<int>")
                .methods("PUT"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: PUT /api/aircraft/" + std::to_string(id));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to update aircraft");
                    }

                    auto response = AircraftController::updateAircraft(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " PUT /api/aircraft/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/aircraft/<int>")
                .methods("DELETE"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: DELETE /api/aircraft/" + std::to_string(id));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to delete aircraft");
                    }

                    auto response = AircraftController::deleteAircraft(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " DELETE /api/aircraft/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/aircraft/<int>/flights")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/aircraft/" + std::to_string(id) + "/flights");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access these flights");
                    }

                    auto response = AircraftController::getAircraftFlights(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/aircraft/" + std::to_string(id) + "/flights");
                    return response;
                });

            // Crew Members routes
            CROW_ROUTE(app, "/api/crew-members")
                .methods("GET"_method)
                ([](const crow::request& req) {
                    LOG_INFO("Request: GET /api/crew-members");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access crew members");
                    }

                    auto response = CrewMemberController::getCrewMembers(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crew-members");
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members")
                .methods("POST"_method)
                ([](const crow::request& req) {
                    LOG_INFO("Request: POST /api/crew-members");

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to create crew members");
                    }

                    auto response = CrewMemberController::createCrewMember(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/crew-members");
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members/<int>")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crew-members/" + std::to_string(id));

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access this crew member");
                    }

                    auto response = CrewMemberController::getCrewMember(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crew-members/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members/<int>")
                .methods("PUT"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: PUT /api/crew-members/" + std::to_string(id));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to update crew member");
                    }

                    auto response = CrewMemberController::updateCrewMember(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " PUT /api/crew-members/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members/<int>")
                .methods("DELETE"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: DELETE /api/crew-members/" + std::to_string(id));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to delete crew member");
                    }

                    auto response = CrewMemberController::deleteCrewMember(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " DELETE /api/crew-members/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members/<int>/assignments")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crew-members/" + std::to_string(id) + "/assignments");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access these assignments");
                    }

                    auto response = CrewMemberController::getCrewMemberAssignments(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crew-members/" + std::to_string(id) + "/assignments");
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members/<int>/flights")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crew-members/" + std::to_string(id) + "/flights");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access these flights");
                    }

                    auto response = CrewMemberController::getCrewMemberFlights(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crew-members/" + std::to_string(id) + "/flights");
                    return response;
                });

            CROW_ROUTE(app, "/api/crew-members/search/<string>")
                .methods("GET"_method)
                ([](const crow::request& req, std::string lastName) {
                    LOG_INFO("Request: GET /api/crew-members/search/" + lastName);

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to search crew members");
                    }

                    auto response = CrewMemberController::searchCrewMembersByLastName(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crew-members/search/" + lastName);
                    return response;
                });

            // Crews routes
            CROW_ROUTE(app, "/api/crews")
                .methods("GET"_method)
                ([](const crow::request& req) {
                    LOG_INFO("Request: GET /api/crews");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access crews");
                    }

                    auto response = CrewController::getCrews(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crews");
                    return response;
                });

            CROW_ROUTE(app, "/api/crews")
                .methods("POST"_method)
                ([](const crow::request& req) {
                    LOG_INFO("Request: POST /api/crews");

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to create crews");
                    }

                    auto response = CrewController::createCrew(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/crews");
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crews/" + std::to_string(id));

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access this crew");
                    }

                    auto response = CrewController::getCrew(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crews/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>")
                .methods("PUT"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: PUT /api/crews/" + std::to_string(id));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to update crew");
                    }

                    auto response = CrewController::updateCrew(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " PUT /api/crews/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>")
                .methods("DELETE"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: DELETE /api/crews/" + std::to_string(id));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to delete crew");
                    }

                    auto response = CrewController::deleteCrew(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " DELETE /api/crews/" + std::to_string(id));
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>/validate")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crews/" + std::to_string(id) + "/validate");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to validate this crew");
                    }

                    auto response = CrewController::validateCrew(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crews/" + std::to_string(id) + "/validate");
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>/members")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crews/" + std::to_string(id) + "/members");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access crew members");
                    }

                    auto response = CrewController::getCrewMembers(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crews/" + std::to_string(id) + "/members");
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>/members")
                .methods("POST"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: POST /api/crews/" + std::to_string(id) + "/members");

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to assign crew members");
                    }

                    auto response = CrewController::assignCrewMember(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/crews/" + std::to_string(id) + "/members");
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>/members/<int>")
                .methods("DELETE"_method)
                ([](const crow::request& req, int id, int memberId) {
                    LOG_INFO("Request: DELETE /api/crews/" + std::to_string(id) + "/members/" + std::to_string(memberId));

                    if (!has_role(req, {"admin"})) {
                        return auth_error(403, "Not authorized to remove crew members");
                    }

                    auto response = CrewController::removeCrewMember(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " DELETE /api/crews/" + std::to_string(id) + "/members/" + std::to_string(memberId));
                    return response;
                });

            CROW_ROUTE(app, "/api/crews/<int>/aircraft")
                .methods("GET"_method)
                ([](const crow::request& req, int id) {
                    LOG_INFO("Request: GET /api/crews/" + std::to_string(id) + "/aircraft");

                    if (!has_role(req, {"admin", "worker"})) {
                        return auth_error(403, "Not authorized to access crew aircraft");
                    }

                    auto response = CrewController::getCrewAircraft(req);
                    LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/crews/" + std::to_string(id) + "/aircraft");
                    return response;
                });
                CROW_ROUTE(app, "/api/flights")
                    .methods("GET"_method)
                    ([](const crow::request& req) {
                        LOG_INFO("Request: GET /api/flights");
                        auto response = FlightController::getFlights(req);
                        LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/flights");
                        return response;
                    });

                CROW_ROUTE(app, "/api/flights")
                    .methods("POST"_method)
                    ([](const crow::request& req) {
                        LOG_INFO("Request: POST /api/flights");

                        if (!has_role(req, {"admin", "worker"})) {
                            return auth_error(403, "Not authorized to create flights");
                        }

                        auto response = FlightController::createFlight(req);
                        LOG_INFO("Response: " + std::to_string(response.code) + " POST /api/flights");
                        return response;
                    });

                CROW_ROUTE(app, "/api/flights/<int>")
                    .methods("GET"_method)
                    ([](const crow::request& req, int id) {
                        LOG_INFO("Request: GET /api/flights/" + std::to_string(id));
                        auto response = FlightController::getFlight(req);
                        LOG_INFO("Response: " + std::to_string(response.code) + " GET /api/flights/" + std::to_string(id));
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
