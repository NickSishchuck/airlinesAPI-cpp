#include "../../include/middleware/AuthMiddleware.h"
#include "../../include/utils/Logger.h"
#include <nlohmann/json.hpp>

crow::response AuthMiddleware::protect(const crow::request& req, std::shared_ptr<void>& context) {
    try {
        // Get token from Authorization header
        std::string authHeader = req.get_header_value("Authorization");

        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            nlohmann::json error;
            error["success"] = false;
            error["error"] = "Not authorized to access this route";
            return crow::response(401, error.dump(4));
        }

        std::string token = authHeader.substr(7);

        // Verify token
        std::unordered_map<std::string, std::string> payload;
        if (!JWTUtils::getInstance().verifyToken(token, payload)) {
            nlohmann::json error;
            error["success"] = false;
            error["error"] = "Not authorized to access this route";
            return crow::response(401, error.dump(4));
        }

        // Check if user still exists
        auto db = DBConnectionPool::getInstance().getConnection();
        auto stmt = db->prepareStatement("SELECT * FROM users WHERE user_id = ?");
        stmt->setInt(1, std::stoi(payload["id"]));
        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            nlohmann::json error;
            error["success"] = false;
            error["error"] = "User no longer exists";
            return crow::response(401, error.dump(4));
        }

        // Store user in context
        auto authContext = std::make_shared<AuthContext>();
        authContext->user = payload;
        authContext->authenticated = true;
        context = authContext;

        // No error, continue to route handler
        return crow::response();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in protect middleware: " + std::string(e.what()));

        nlohmann::json error;
        error["success"] = false;
        error["error"] = "Not authorized to access this route";

        return crow::response(401, error.dump(4));
    }
}

crow::response AuthMiddleware::authorize(const crow::request& req,
                                     std::shared_ptr<void>& context,
                                     const std::unordered_set<std::string>& roles) {
    try {
        // Make sure context is an AuthContext
        auto authContext = std::dynamic_pointer_cast<AuthContext>(context);
        if (!authContext || !authContext->authenticated) {
            nlohmann::json error;
            error["success"] = false;
            error["error"] = "Not authorized to access this route";
            return crow::response(401, error.dump(4));
        }

        // Check if user has required role
        if (roles.find(authContext->user["role"]) == roles.end()) {
            nlohmann::json error;
            error["success"] = false;
            error["error"] = "User role '" + authContext->user["role"] + "' is not authorized to access this route";
            return crow::response(403, error.dump(4));
        }

        // No error, continue to route handler
        return crow::response();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in authorize middleware: " + std::string(e.what()));

        nlohmann::json error;
        error["success"] = false;
        error["error"] = "Not authorized to access this route";

        return crow::response(401, error.dump(4));
    }
}
