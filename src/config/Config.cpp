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
            std::cerr << "Could not open configuration file: " << filename << std::endl;
            Logger::error("Could not open configuration file: " + filename);
            return false;
        }

        // Check file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::cerr << "Config file size: " << fileSize << " bytes" << std::endl;

        // Read raw contents for debugging
        std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::cerr << "Raw config content: " << std::endl << rawContent << std::endl;
        
        // Reset file position for parsing
        file.clear();
        file.seekg(0, std::ios::beg);

        // Parse the JSON
        json config;
        try {
            file >> config;
            std::cerr << "Successfully parsed JSON" << std::endl;
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            Logger::error("JSON parse error: " + std::string(e.what()));
            return false;
        }

        // Debug output the full config
        std::cerr << "Parsed JSON: " << config.dump(2) << std::endl;

        // Load values from JSON
        if (config.contains("port")) {
            port = config["port"].get<int>();
            std::cerr << "Loaded port: " << port << std::endl;
        } else {
            std::cerr << "Config does not contain 'port'" << std::endl;
        }

        if (config.contains("database")) {
            auto& db = config["database"];
            std::cerr << "Database section: " << db.dump(2) << std::endl;
            
            if (db.contains("host")) {
                dbHost = db["host"].get<std::string>();
                std::cerr << "Loaded dbHost: " << dbHost << std::endl;
            } else {
                std::cerr << "Database does not contain 'host'" << std::endl;
            }
            
            if (db.contains("user")) {
                dbUser = db["user"].get<std::string>();
                std::cerr << "Loaded dbUser: " << dbUser << std::endl;
            } else {
                std::cerr << "Database does not contain 'user'" << std::endl;
            }
            
            if (db.contains("password")) {
                dbPassword = db["password"].get<std::string>();
                std::cerr << "Loaded dbPassword: " << dbPassword << std::endl;
            } else {
                std::cerr << "Database does not contain 'password'" << std::endl;
            }
            
            if (db.contains("name")) {
                dbName = db["name"].get<std::string>();
                std::cerr << "Loaded dbName: " << dbName << std::endl;
            } else {
                std::cerr << "Database does not contain 'name'" << std::endl;
            }
            
            if (db.contains("port")) {
                dbPort = db["port"].get<int>();
                std::cerr << "Loaded dbPort: " << dbPort << std::endl;
            } else {
                std::cerr << "Database does not contain 'port'" << std::endl;
            }
            
            if (db.contains("poolSize")) {
                dbPoolSize = db["poolSize"].get<int>();
                std::cerr << "Loaded dbPoolSize: " << dbPoolSize << std::endl;
            } else {
                std::cerr << "Database does not contain 'poolSize'" << std::endl;
            }
        } else {
            std::cerr << "Config does not contain 'database' section" << std::endl;
        }

        // Print default values vs loaded values
        std::cerr << "\nCurrent configuration after loading:" << std::endl;
        std::cerr << "port: " << port << std::endl;
        std::cerr << "dbHost: " << dbHost << std::endl;
        std::cerr << "dbUser: " << dbUser << std::endl;
        std::cerr << "dbPassword: " << dbPassword << std::endl;
        std::cerr << "dbName: " << dbName << std::endl;
        std::cerr << "dbPort: " << dbPort << std::endl;
        std::cerr << "dbPoolSize: " << dbPoolSize << std::endl;

        Logger::info("Configuration loaded successfully from " + filename);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        Logger::error("Error loading configuration: " + std::string(e.what()));
        return false;
    }
}
