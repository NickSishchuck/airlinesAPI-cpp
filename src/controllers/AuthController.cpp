#include "../../include/controllers/AuthController.h"
#include "../../include/utils/Logger.h"
#include "../../include/middleware/AuthMiddleware.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sstream>
#include <regex>
#include <iomanip>
#include <openssl/sha.h>

using json = nlohmann::json;

// Simple password hashing using SHA-256 for now
// In a production system, use a more secure hashing algorithm with salt like bcrypt
std::string AuthController::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool AuthController::verifyPassword(const std::string& providedPassword, const std::string& storedHash) {
    // For now, the JavaScript API is using plaintext passwords as noted in the code
    // So we'll match that behavior, but in production this should be a secure comparison
    return providedPassword == storedHash;
}

crow::response AuthController::createTokenResponse(int userId, const std::string& role, const nlohmann::json& userData) {
    try {
        // Generate JWT token
        std::string token = JWTUtils::getInstance().generateToken(userId, role);

        // Create response JSON
        json response;
        response["success"] = true;
        response["token"] = token;
        response["data"] = userData;

        return crow::response(200, response.dump(4));
    } catch (const std::exception& e) {
        LOG_ERROR("Error creating token response: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::registerEmail(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate required fields
        if (!requestData.contains("name") || !requestData.contains("email") || !requestData.contains("password")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide name, email and password";
            return crow::response(400, error.dump(4));
        }

        std::string name = requestData["name"];
        std::string email = requestData["email"];
        std::string password = requestData["password"];
        std::string role = requestData.contains("role") ? requestData["role"] : "user";

        // Validate email format
        std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        if (!std::regex_match(email, emailRegex)) {
            json error;
            error["success"] = false;
            error["error"] = "Invalid email format";
            return crow::response(400, error.dump(4));
        }

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if email already exists
        auto stmt = db->prepareStatement("SELECT * FROM users WHERE email = ?");
        stmt->setString(1, email);
        auto result = db->executeQuery(stmt);

        if (result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Email already in use";
            return crow::response(400, error.dump(4));
        }

        // Create user
        auto insertStmt = db->prepareStatement(
            "INSERT INTO users (first_name, email, password, role) VALUES (?, ?, ?, ?)"
        );
        insertStmt->setString(1, name);
        insertStmt->setString(2, email);
        insertStmt->setString(3, password); // plaintext to match JS API behavior
        insertStmt->setString(4, role);

        db->executeUpdate(insertStmt);
        // Get the last insert ID
        auto idStmt = db->prepareStatement("SELECT LAST_INSERT_ID()");
        auto idResult = db->executeQuery(idStmt);
        idResult->next();
        int userId = idResult->getInt(1);

        // Get created user
        auto userStmt = db->prepareStatement(
            "SELECT user_id, first_name, email, role, created_at FROM users WHERE user_id = ?"
        );
        userStmt->setInt(1, userId);
        auto userResult = db->executeQuery(userStmt);

        if (!userResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Error retrieving user data";
            return crow::response(500, error.dump(4));
        }

        // Create user data JSON
        json userData;
        userData["user_id"] = userResult->getInt("user_id");
        userData["first_name"] = userResult->getString("first_name");
        userData["email"] = userResult->getString("email");
        userData["role"] = userResult->getString("role");
        userData["created_at"] = userResult->getString("created_at");

        // Create token response
        return createTokenResponse(userId, role, userData);
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in registerEmail: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in registerEmail: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::registerPhone(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate required fields
        if (!requestData.contains("name") || !requestData.contains("phone") || !requestData.contains("password")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide name, phone and password";
            return crow::response(400, error.dump(4));
        }

        std::string name = requestData["name"];
        std::string phone = requestData["phone"];
        std::string password = requestData["password"];
        std::string role = requestData.contains("role") ? requestData["role"] : "user";

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check if phone already exists
        auto stmt = db->prepareStatement("SELECT * FROM users WHERE contact_number = ?");
        stmt->setString(1, phone);
        auto result = db->executeQuery(stmt);

        if (result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Phone already in use";
            return crow::response(400, error.dump(4));
        }

        // Create user
        auto insertStmt = db->prepareStatement(
            "INSERT INTO users (first_name, contact_number, password, role) VALUES (?, ?, ?, ?)"
        );
        insertStmt->setString(1, name);
        insertStmt->setString(2, phone);
        insertStmt->setString(3, password); // plaintext to match JS API behavior
        insertStmt->setString(4, role);

        db->executeUpdate(insertStmt);
        // Get the last insert ID
        auto idStmt = db->prepareStatement("SELECT LAST_INSERT_ID()");
        auto idResult = db->executeQuery(idStmt);
        idResult->next();
        int userId = idResult->getInt(1);

        // Get created user
        auto userStmt = db->prepareStatement(
            "SELECT user_id, first_name, contact_number, role, created_at FROM users WHERE user_id = ?"
        );
        userStmt->setInt(1, userId);
        auto userResult = db->executeQuery(userStmt);

        if (!userResult->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Error retrieving user data";
            return crow::response(500, error.dump(4));
        }

        // Create user data JSON
        json userData;
        userData["user_id"] = userResult->getInt("user_id");
        userData["first_name"] = userResult->getString("first_name");
        userData["contact_number"] = userResult->getString("contact_number");
        userData["role"] = userResult->getString("role");
        userData["created_at"] = userResult->getString("created_at");

        // Create token response
        return createTokenResponse(userId, role, userData);
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in registerPhone: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in registerPhone: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::login(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate email & password
        if (!requestData.contains("email") || !requestData.contains("password")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide email and password";
            return crow::response(400, error.dump(4));
        }

        std::string email = requestData["email"];
        std::string password = requestData["password"];

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check for user with direct password comparison (matching JS API behavior)
        auto stmt = db->prepareStatement(
            "SELECT * FROM users WHERE email = ? AND password = ?"
        );
        stmt->setString(1, email);
        stmt->setString(2, password);
        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Invalid credentials";
            return crow::response(401, error.dump(4));
        }

        // Get user data
        int userId = result->getInt("user_id");
        std::string role = static_cast<std::string>(result->getString("role"));

        // Create user data JSON
        json userData;
        userData["user_id"] = userId;
        userData["first_name"] = result->getString("first_name");
        userData["email"] = result->getString("email");
        userData["role"] = role;

        if (result->getMetaData()->getColumnCount() > 4) {
            // Add optional fields if they exist
            if (result->findColumn("last_name") != 0) {
                userData["last_name"] = result->getString("last_name");
            }
            if (result->findColumn("created_at") != 0) {
                userData["created_at"] = result->getString("created_at");
            }
        }

        // Create token response
        return createTokenResponse(userId, role, userData);
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in login: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in login: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::loginPhone(const crow::request& req) {
    try {
        // Parse request body
        json requestData = json::parse(req.body);

        // Validate phone & password
        if (!requestData.contains("phone") || !requestData.contains("password")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide phone and password";
            return crow::response(400, error.dump(4));
        }

        std::string phone = requestData["phone"];
        std::string password = requestData["password"];

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check for user with direct password comparison (matching JS API behavior)
        auto stmt = db->prepareStatement(
            "SELECT * FROM users WHERE contact_number = ? AND password = ?"
        );
        stmt->setString(1, phone);
        stmt->setString(2, password);
        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Invalid credentials";
            return crow::response(401, error.dump(4));
        }

        // Get user data
        int userId = result->getInt("user_id");
        std::string role = result->getString("role").c_str();

        // Create user data JSON
        json userData;
        userData["user_id"] = userId;
        userData["first_name"] = result->getString("first_name");
        userData["contact_number"] = result->getString("contact_number");
        userData["role"] = role;

        if (result->getMetaData()->getColumnCount() > 4) {
            // Add optional fields if they exist
            if (result->findColumn("last_name") != 0) {
                userData["last_name"] = result->getString("last_name");
            }
            if (result->findColumn("created_at") != 0) {
                userData["created_at"] = result->getString("created_at");
            }
        }

        // Create token response
        return createTokenResponse(userId, role, userData);
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in loginPhone: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in loginPhone: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::getMe(const crow::request& req) {
    try {
        // Extract user ID from the authenticated user context
        int userId = get_user_id(req);

        // Get database connection to fetch latest user data
        auto db = DBConnectionPool::getInstance().getConnection();

        // Get user data from database
        auto stmt = db->prepareStatement(
            "SELECT user_id, first_name, last_name, email, contact_number, role, created_at FROM users WHERE user_id = ?"
        );
        stmt->setInt(1, userId);
        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "User not found";
            return crow::response(404, error.dump(4));
        }

        // Create response
        json response;
        response["success"] = true;

        // Build user data JSON
        json userObj;
        userObj["user_id"] = result->getInt("user_id");
        userObj["first_name"] = result->getString("first_name");

        if (!result->isNull("last_name")) {
            userObj["last_name"] = result->getString("last_name");
        }

        if (!result->isNull("email")) {
            userObj["email"] = result->getString("email");
        }

        if (!result->isNull("contact_number")) {
            userObj["contact_number"] = result->getString("contact_number");
        }

        userObj["role"] = result->getString("role");
        userObj["created_at"] = result->getString("created_at");

        response["data"] = userObj;

        return crow::response(200, response.dump(4));
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in getMe: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in getMe: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::updatePassword(const crow::request& req) {
    try {

        // Parse request body
        json requestData = json::parse(req.body);

        // Validate inputs
        if (!requestData.contains("currentPassword") || !requestData.contains("newPassword")) {
            json error;
            error["success"] = false;
            error["error"] = "Please provide current and new password";
            return crow::response(400, error.dump(4));
        }

        std::string currentPassword = requestData["currentPassword"];
        std::string newPassword = requestData["newPassword"];

        int userId = get_user_id(req);

        // Get database connection
        auto db = DBConnectionPool::getInstance().getConnection();

        // Check current password
        auto stmt = db->prepareStatement(
            "SELECT * FROM users WHERE user_id = ? AND password = ?"
        );
        stmt->setInt(1, userId);
        stmt->setString(2, currentPassword);
        auto result = db->executeQuery(stmt);

        if (!result->next()) {
            json error;
            error["success"] = false;
            error["error"] = "Current password is incorrect";
            return crow::response(401, error.dump(4));
        }

        // Update password
        auto updateStmt = db->prepareStatement(
            "UPDATE users SET password = ? WHERE user_id = ?"
        );
        updateStmt->setString(1, newPassword);
        updateStmt->setInt(2, userId);
        db->executeUpdate(updateStmt);

        // Generate new token
        std::string role = result->getString("role").c_str();
        std::string token = JWTUtils::getInstance().generateToken(userId, role);

        // Create response
        json response;
        response["success"] = true;
        response["token"] = token;
        response["message"] = "Password updated successfully";

        return crow::response(200, response.dump(4));
    }
    catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Invalid JSON format";

        return crow::response(400, error.dump(4));
    }
    catch (const sql::SQLException& e) {
        LOG_ERROR("SQL error in updatePassword: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = "Database error";

        return crow::response(500, error.dump(4));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in updatePassword: " + std::string(e.what()));

        json error;
        error["success"] = false;
        error["error"] = e.what();

        return crow::response(500, error.dump(4));
    }
}

crow::response AuthController::logout(const crow::request& req) {
    // In this implementation, we don't need to do anything on the server side
    // for logout since we're using JWT. The client should just remove the token.
    json response;
    response["success"] = true;
    response["data"] = json::object();

    return crow::response(200, response.dump(4));
}
