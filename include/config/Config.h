#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Config {
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    bool load(const std::string& filename);

    // Getters for configuration values
    int getPort() const { return port; }
    std::string getDbHost() const { return dbHost; }
    std::string getDbUser() const { return dbUser; }
    std::string getDbPassword() const { return dbPassword; }
    std::string getDbName() const { return dbName; }
    int getDbPort() const { return dbPort; }
    int getDbPoolSize() const { return dbPoolSize; }
    std::string getJwtSecret() const { return jwtSecret; }
    int getJwtExpiresIn() const { return jwtExpiresIn; }

private:
    Config() = default;
    ~Config() = default;

    // Disable copy and move
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    // Configuration values
    int port = 3000;
    std::string dbHost = "localhost";
    std::string dbUser = "airline_user";
    std::string dbPassword = "airline_password";
    std::string dbName = "airline_transportation";
    int dbPort = 3306;
    int dbPoolSize = 10;
    std::string jwtSecret = "simpleSecretKey123";
    int jwtExpiresIn = 2592000; // 30 days in seconds
};
