#include "../../include/utils/JWTUtils.h"
#include "../../include/utils/Logger.h"
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <stdexcept>
#include <sstream>

std::string JWTUtils::generateToken(int userId, const std::string& role) {
    try {
        // Get current time for issued-at claim
        auto now = std::chrono::system_clock::now();

        // Create JWT token
        auto token = jwt::create()
            .set_issuer("airline-api")
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::seconds(expiresIn))
            .set_payload_claim("id", jwt::claim(std::to_string(userId)))
            .set_payload_claim("role", jwt::claim(role))
            .sign(jwt::algorithm::hs256{secret});

        return token;
    } catch (const std::exception& e) {
        LOG_ERROR("Error generating JWT token: " + std::string(e.what()));
        throw std::runtime_error("Failed to generate authentication token");
    }
}

bool JWTUtils::verifyToken(const std::string& token, std::unordered_map<std::string, std::string>& payload) {
    try {
        // Verify and decode token
        auto decoded = jwt::decode(token);

        // Verify the token signature and validate the expiration time
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .with_issuer("airline-api");

        verifier.verify(decoded);

        // Extract payload claims
        payload["id"] = decoded.get_payload_claim("id").as_string();
        payload["role"] = decoded.get_payload_claim("role").as_string();

        return true;
    } catch (const jwt::error::token_verification_exception& e) {
        LOG_ERROR("JWT token verification failed: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error verifying JWT token: " + std::string(e.what()));
        return false;
    }
}
