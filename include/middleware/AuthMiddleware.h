#pragma once

#include <crow.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../utils/JWTUtils.h"
#include "../database/DBConnectionPool.h"

// Use nlohmann::json explicitly
using json = nlohmann::json;

// Authentication middleware for Crow
struct AuthMiddleware {
    struct context {
        std::unordered_map<std::string, std::string> user;
        bool authenticated = false;
        std::string role;
        int user_id = 0;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        try {
            // Get token from Authorization header
            std::string authHeader = req.get_header_value("Authorization");

            if (!authHeader.empty() && authHeader.substr(0, 7) == "Bearer ") {
                std::string token = authHeader.substr(7);

                // Verify token
                std::unordered_map<std::string, std::string> payload;
                if (JWTUtils::getInstance().verifyToken(token, payload)) {
                    ctx.user = payload;
                    ctx.authenticated = true;

                    // Store role and user_id for easier access
                    if (payload.count("role")) {
                        ctx.role = payload["role"];
                    }

                    if (payload.count("id")) {
                        ctx.user_id = std::stoi(payload["id"]);
                    }

                    // Check if user still exists in the database
                    try {
                        auto db = DBConnectionPool::getInstance().getConnection();
                        auto stmt = db->prepareStatement("SELECT * FROM users WHERE user_id = ?");
                        stmt->setInt(1, ctx.user_id);
                        auto result = db->executeQuery(stmt);

                        if (!result->next()) {
                            ctx.authenticated = false;
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR("Database error in auth middleware: " + std::string(e.what()));
                        // Continue with the authentication we have
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Auth middleware error: " + std::string(e.what()));
            // Continue processing
        }
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Nothing to do after handling
    }
};

// Since we can't directly access the middleware context, let's create a lightweight auth system
// that doesn't depend on the middleware context, but rather processes the request directly

/**
 * Helper function to check if a request is authenticated
 */
inline bool is_authenticated(const crow::request& req) {
    std::string authHeader = req.get_header_value("Authorization");

    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        return false;
    }

    std::string token = authHeader.substr(7);
    std::unordered_map<std::string, std::string> payload;

    return JWTUtils::getInstance().verifyToken(token, payload);
}

/**
 * Helper function to extract user data from a request
 */
inline std::unordered_map<std::string, std::string> get_user_data(const crow::request& req) {
    std::unordered_map<std::string, std::string> payload;
    std::string authHeader = req.get_header_value("Authorization");

    if (!authHeader.empty() && authHeader.substr(0, 7) == "Bearer ") {
        std::string token = authHeader.substr(7);
        JWTUtils::getInstance().verifyToken(token, payload);
    }

    return payload;
}

/**
 * Helper function to check if a user has the required role
 */
inline bool has_role(const crow::request& req, const std::vector<std::string>& roles) {
    auto userData = get_user_data(req);

    // If not authenticated or no role info
    if (userData.empty() || userData.find("role") == userData.end()) {
        return false;
    }

    // If no roles specified, any authenticated user is allowed
    if (roles.empty()) {
        return true;
    }

    // Check if user has any of the required roles
    std::string userRole = userData["role"];
    for (const auto& role : roles) {
        if (userRole == role) {
            return true;
        }
    }

    return false;
}

/**
 * Helper function to get user ID from request
 */
inline int get_user_id(const crow::request& req) {
    auto userData = get_user_data(req);

    if (userData.empty() || userData.find("id") == userData.end()) {
        return 0;
    }

    return std::stoi(userData["id"]);
}

/**
 * Helper function to create an error response
 */
inline crow::response auth_error(int status, const std::string& message) {
    nlohmann::json error;
    error["success"] = false;
    error["error"] = message;
    return crow::response(status, error.dump(4));
}
