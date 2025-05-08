#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <mariadb/conncpp.hpp>

class DBConnection {
public:
    DBConnection(std::shared_ptr<sql::Connection> conn);
    ~DBConnection();

    // Get raw connection
    std::shared_ptr<sql::Connection> getConnection() const { return connection; }

    // Query execution helpers
    std::unique_ptr<sql::ResultSet> executeQuery(const std::string& query);
    std::unique_ptr<sql::ResultSet> executeQuery(std::unique_ptr<sql::PreparedStatement>& stmt);
    int executeUpdate(const std::string& query);
    int executeUpdate(std::unique_ptr<sql::PreparedStatement>& stmt);
    std::unique_ptr<sql::PreparedStatement> prepareStatement(const std::string& query);

private:
    std::shared_ptr<sql::Connection> connection;
    bool inUse;

    friend class DBConnectionPool;
};

class DBConnectionPool {
public:
    static DBConnectionPool& getInstance() {
        static DBConnectionPool instance;
        return instance;
    }

    // Initialize the pool
    bool initialize(
        const std::string& host,
        const std::string& user,
        const std::string& password,
        const std::string& database,
        int port = 3306,
        int poolSize = 10
    );

    // Get a database connection from the pool
    std::shared_ptr<DBConnection> getConnection();

    // Check database health
    bool checkHealth();

    // Close all connections and clean up
    void cleanup();

private:
    DBConnectionPool();
    ~DBConnectionPool();

    // Disable copy and move
    DBConnectionPool(const DBConnectionPool&) = delete;
    DBConnectionPool& operator=(const DBConnectionPool&) = delete;
    DBConnectionPool(DBConnectionPool&&) = delete;
    DBConnectionPool& operator=(DBConnectionPool&&) = delete;

    // Create a new database connection
    std::shared_ptr<sql::Connection> createConnection();

    std::shared_ptr<sql::Driver> driver;
    std::vector<std::shared_ptr<DBConnection>> connections;
    std::mutex mutex;

    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int port;
    bool initialized;
};
