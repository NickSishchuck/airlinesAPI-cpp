#include "../../include/config/Config.h"
#include "../../include/utils/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

bool Config::load(const std::string& filename) {
    try {
        // Read the JSON configuration file
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Could not open configuration file: " + filename);
            return false;
        }

        // Check file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        LOG_DEBUG("Config file size: " + std::to_string(fileSize) + " bytes");

        // Read raw contents for debugging
        std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        LOG_DEBUG("Raw config content: \n" + rawContent);

        // Reset file position for parsing
        file.clear();
        file.seekg(0, std::ios::beg);

        // Parse the JSON
        json config;
        try {
            file >> config;
            LOG_DEBUG("Successfully parsed JSON");
        } catch (const json::parse_error& e) {
            LOG_ERROR("JSON parse error: " + std::string(e.what()));
            return false;
        }

        // Debug output the full config
        LOG_DEBUG("Parsed JSON: " + config.dump(2));

        // Load values from JSON
        if (config.contains("port")) {
            port = config["port"].get<int>();
            LOG_DEBUG("Loaded port: " + std::to_string(port));
        } else {
            LOG_WARNING("Config does not contain 'port'");
        }

        if (config.contains("database")) {
            auto& db = config["database"];
            LOG_DEBUG("Database section: " + db.dump(2));

            if (db.contains("host")) {
                dbHost = db["host"].get<std::string>();
                LOG_DEBUG("Loaded dbHost: " + dbHost);
            } else {
                LOG_WARNING("Database does not contain 'host'");
            }

            if (db.contains("user")) {
                dbUser = db["user"].get<std::string>();
                LOG_DEBUG("Loaded dbUser: " + dbUser);
            } else {
                LOG_WARNING("Database does not contain 'user'");
            }

            if (db.contains("password")) {
                dbPassword = db["password"].get<std::string>();
                LOG_DEBUG("Loaded dbPassword: " + dbPassword);
            } else {
                LOG_WARNING("Database does not contain 'password'");
            }

            if (db.contains("name")) {
                dbName = db["name"].get<std::string>();
                LOG_DEBUG("Loaded dbName: " + dbName);
            } else {
                LOG_WARNING("Database does not contain 'name'");
            }

            if (db.contains("port")) {
                dbPort = db["port"].get<int>();
                LOG_DEBUG("Loaded dbPort: " + std::to_string(dbPort));
            } else {
                LOG_WARNING("Database does not contain 'port'");
            }

            if (db.contains("poolSize")) {
                dbPoolSize = db["poolSize"].get<int>();
                LOG_DEBUG("Loaded dbPoolSize: " + std::to_string(dbPoolSize));
            } else {
                LOG_WARNING("Database does not contain 'poolSize'");
            }
        } else {
            LOG_WARNING("Config does not contain 'database' section");
        }

        // Load JWT configuration
        if (config.contains("jwt")) {
            auto& jwt = config["jwt"];
            LOG_DEBUG("JWT section: " + jwt.dump(2));

            if (jwt.contains("secret")) {
                jwtSecret = jwt["secret"].get<std::string>();
                LOG_DEBUG("Loaded jwtSecret");
            } else {
                LOG_WARNING("JWT does not contain 'secret'");
            }

            if (jwt.contains("expiresIn")) {
                jwtExpiresIn = jwt["expiresIn"].get<int>();
                LOG_DEBUG("Loaded jwtExpiresIn: " + std::to_string(jwtExpiresIn));
            } else {
                LOG_WARNING("JWT does not contain 'expiresIn'");
            }
        } else {
            LOG_WARNING("Config does not contain 'jwt' section");
        }

        // Print default values vs loaded values
        LOG_INFO("Current configuration after loading:");
        LOG_INFO("port: " + std::to_string(port));
        LOG_INFO("dbHost: " + dbHost);
        LOG_INFO("dbUser: " + dbUser);
        LOG_INFO("dbPassword: " + dbPassword);
        LOG_INFO("dbName: " + dbName);
        LOG_INFO("dbPort: " + std::to_string(dbPort));
        LOG_INFO("dbPoolSize: " + std::to_string(dbPoolSize));
        (jwtSecret.empty() ? LOG_INFO("jwtSecret: Not set") : LOG_INFO("jwtSecret: Set"));
        LOG_INFO("jwtExpiresIn: " + std::to_string(jwtExpiresIn));

        LOG_INFO("Configuration loaded successfully from " + filename);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error loading configuration: " + std::string(e.what()));
        return false;
    }
}
