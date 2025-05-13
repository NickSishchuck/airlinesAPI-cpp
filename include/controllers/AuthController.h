#pragma once

#include <crow.h>
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"
#include "../utils/JWTUtils.h"
#include "../database/DBConnectionPool.h"
#include "../middleware/AuthMiddleware.h"

// Use nlohmann::json explicitly
using json = nlohmann::json;

class AuthController {
public:
    // Password hashing and verification
    static std::string hashPassword(const std::string& password);
    static bool verifyPassword(const std::string& providedPassword, const std::string& storedHash);

    // Helper for creating token responses
    static crow::response createTokenResponse(int userId, const std::string& role, const nlohmann::json& userData);

    // Auth endpoints
    static crow::response registerEmail(const crow::request& req);
    static crow::response registerPhone(const crow::request& req);
    static crow::response login(const crow::request& req);
    static crow::response loginPhone(const crow::request& req);
    static crow::response getMe(const crow::request& req);
    static crow::response updatePassword(const crow::request& req);
    static crow::response logout(const crow::request& req);
};
