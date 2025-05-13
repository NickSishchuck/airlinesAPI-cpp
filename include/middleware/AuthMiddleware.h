#pragma once

#include <crow.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../utils/JWTUtils.h"
#include "../database/DBConnectionPool.h"

// Use nlohmann::json explicitly
using json = nlohmann::json;

// Authentication middleware for Crow routes
class AuthMiddleware {
public:
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

            if (authHeader.substr(0, 7) == "Bearer ") {
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

                            res.code = 401;
                            res.body = json({
                                {"success", false},
                                {"error", "User no longer exists"}
                            }).dump(4);
                            res.end();
                            return;
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR("Database error in auth middleware: " + std::string(e.what()));
                        // Continue with the authentication we have
                    }
                } else {
                    // Invalid token
                    res.code = 401;
                    res.body = json({
                        {"success", false},
                        {"error", "Invalid token"}
                    }).dump(4);
                    res.end();
                    return;
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

/**
 * Helper function to extract the auth context from the request's middleware
 */
inline AuthMiddleware::context& get_auth_context(const crow::request& req) {
    return *reinterpret_cast<AuthMiddleware::context*>(req.middleware_context["AuthMiddleware"]);
}

/**
 * Helper function to check if a user is authenticated
 */
inline bool is_authenticated(const crow::request& req) {
    auto& ctx = get_auth_context(req);
    return ctx.authenticated;
}

/**
 * Helper function to check if a user has the required role
 */
inline bool has_role(const crow::request& req, const std::vector<std::string>& roles) {
    auto& ctx = get_auth_context(req);

    // If no roles specified, any authenticated user is allowed
    if (!ctx.authenticated) {
        return false;
    }

    if (roles.empty()) {
        return true;
    }

    // Check if user has any of the required roles
    for (const auto& role : roles) {
        if (ctx.role == role) {
            return true;
        }
    }

    return false;
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
