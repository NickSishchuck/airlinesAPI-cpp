#include "../../include/config/Config.h"
#include "../../include/utils/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool Config::load(const std::string& filename) {
    try {
        // Read the JSON configuration file
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::error("Could not open configuration file: " + filename);
            return false;
        }

        // Parse the JSON
        json config;
        file >> config;

        // Load values from JSON
        if (config.contains("port")) {
            port = config["port"].get<int>();
        }

        if (config.contains("database")) {
            auto& db = config["database"];
            if (db.contains("host")) dbHost = db["host"].get<std::string>();
            if (db.contains("user")) dbUser = db["user"].get<std::string>();
            if (db.contains("password")) dbPassword = db["password"].get<std::string>();
            if (db.contains("name")) dbName = db["name"].get<std::string>();
            if (db.contains("port")) dbPort = db["port"].get<int>();
            if (db.contains("poolSize")) dbPoolSize = db["poolSize"].get<int>();
        }

        if (config.contains("jwt")) {
            auto& jwt = config["jwt"];
            if (jwt.contains("secret")) jwtSecret = jwt["secret"].get<std::string>();
            if (jwt.contains("expiresIn")) jwtExpiresIn = jwt["expiresIn"].get<int>();
        }

        Logger::info("Configuration loaded successfully from " + filename);
        return true;
    }
    catch (const std::exception& e) {
        Logger::error("Error loading configuration: " + std::string(e.what()));
        return false;
    }
}
