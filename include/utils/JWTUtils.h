#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <jwt-cpp/jwt.h>

class JWTUtils {
public:
    static JWTUtils& getInstance() {
        static JWTUtils instance;
        return instance;
    }

    // Generate JWT token for a user
    std::string generateToken(int userId, const std::string& role = "user");

    // Verify JWT token and return payload if valid
    bool verifyToken(const std::string& token, std::unordered_map<std::string, std::string>& payload);

    // Set secret key from config
    void setSecret(const std::string& secret) { this->secret = secret; }

    // Set token expiration time in seconds
    void setExpiresIn(int expiresIn) { this->expiresIn = expiresIn; }

private:
    JWTUtils() : expiresIn(2592000) {} // Default 30 days
    ~JWTUtils() = default;

    // Disable copy and move
    JWTUtils(const JWTUtils&) = delete;
    JWTUtils& operator=(const JWTUtils&) = delete;
    JWTUtils(JWTUtils&&) = delete;
    JWTUtils& operator=(JWTUtils&&) = delete;

    std::string secret = "simpleSecretKey123"; // Default, should be updated from config
    int expiresIn;
};
