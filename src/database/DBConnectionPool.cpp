#include "../../include/database/DBConnectionPool.h"
#include "../../include/utils/Logger.h"
#include <iostream>
#include <sstream>

// DBConnection implementation
DBConnection::DBConnection(std::shared_ptr<sql::Connection> conn) : connection(conn), inUse(false) {}

DBConnection::~DBConnection() {
    try {
        if (connection) {
            connection->close();
        }
    }
    catch (const sql::SQLException& e) {
        // Just log the error, but don't throw from destructor
        std::stringstream ss;
        ss << "Error closing database connection: " << e.what();
        Logger::error(ss.str());
    }
}

std::unique_ptr<sql::ResultSet> DBConnection::executeQuery(const std::string& query) {
    try {
        std::unique_ptr<sql::Statement> stmt(connection->createStatement());
        return std::unique_ptr<sql::ResultSet>(stmt->executeQuery(query));
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error in executeQuery: " << e.what() << ". Query: " << query;
        Logger::error(ss.str());
        throw;
    }
}

std::unique_ptr<sql::ResultSet> DBConnection::executeQuery(std::unique_ptr<sql::PreparedStatement>& stmt) {
    try {
        return std::unique_ptr<sql::ResultSet>(stmt->executeQuery());
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error in executeQuery with prepared statement: " << e.what();
        Logger::error(ss.str());
        throw;
    }
}

int DBConnection::executeUpdate(const std::string& query) {
    try {
        std::unique_ptr<sql::Statement> stmt(connection->createStatement());
        return stmt->executeUpdate(query);
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error in executeUpdate: " << e.what() << ". Query: " << query;
        Logger::error(ss.str());
        throw;
    }
}

int DBConnection::executeUpdate(std::unique_ptr<sql::PreparedStatement>& stmt) {
    try {
        return stmt->executeUpdate();
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error in executeUpdate with prepared statement: " << e.what();
        Logger::error(ss.str());
        throw;
    }
}

std::unique_ptr<sql::PreparedStatement> DBConnection::prepareStatement(const std::string& query) {
    try {
        return std::unique_ptr<sql::PreparedStatement>(connection->prepareStatement(query));
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error in prepareStatement: " << e.what() << ". Query: " << query;
        Logger::error(ss.str());
        throw;
    }
}

// DBConnectionPool implementation
DBConnectionPool::DBConnectionPool() : initialized(false) {}

DBConnectionPool::~DBConnectionPool() {
    cleanup();
}

bool DBConnectionPool::initialize(
    const std::string& host,
    const std::string& user,
    const std::string& password,
    const std::string& database,
    int port,
    int poolSize
) {
    std::lock_guard<std::mutex> lock(mutex);

    if (initialized) {
        return true;
    }

    try {
        // Store connection parameters
        this->host = host;
        this->user = user;
        this->password = password;
        this->database = database;
        this->port = port;

        // Get the MariaDB driver
        // Note: MariaDB's driver should not be deleted, so we use a shared_ptr with a no-op deleter
        sql::Driver* rawDriver = sql::mariadb::get_driver_instance();
        driver = std::shared_ptr<sql::Driver>(rawDriver, [](sql::Driver*) {
            // No-op deleter because driver instance is managed by MariaDB internally
        });

        // Create the initial pool of connections
        for (int i = 0; i < poolSize; ++i) {
            auto conn = createConnection();
            if (conn) {
                connections.push_back(std::make_shared<DBConnection>(conn));
            }
        }

        if (connections.empty()) {
            Logger::error("Failed to create any database connections");
            return false;
        }

        initialized = true;
        Logger::info("Database connection pool initialized with " +
                     std::to_string(connections.size()) + " connections");
        return true;
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error initializing connection pool: " << e.what();
        Logger::error(ss.str());
        return false;
    }
    catch (const std::exception& e) {
        Logger::error("Error initializing connection pool: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<DBConnection> DBConnectionPool::getConnection() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!initialized) {
        throw std::runtime_error("Database connection pool not initialized");
    }

    // Find an available connection
    for (auto& conn : connections) {
        if (!conn->inUse) {
            conn->inUse = true;
            return conn;
        }
    }

    // If all connections are in use, create a new one
    try {
        auto newConn = createConnection();
        if (!newConn) {
            throw std::runtime_error("Failed to create a new database connection");
        }

        auto conn = std::make_shared<DBConnection>(newConn);
        conn->inUse = true;
        connections.push_back(conn);

        Logger::info("Created a new database connection. Pool size: " +
                     std::to_string(connections.size()));

        return conn;
    }
    catch (const std::exception& e) {
        Logger::error("Error creating a new database connection: " + std::string(e.what()));
        throw;
    }
}

// Completely rewritten checkHealth method for src/database/DBConnectionPool.cpp
bool DBConnectionPool::checkHealth() {
    std::shared_ptr<DBConnection> conn = nullptr;
    
    try {
        // First, check if we're initialized
        if (!initialized) {
            Logger::error("Database health check failed: Connection pool not initialized");
            return false;
        }
        
        // Step 1: Get a connection (with locking to ensure thread safety)
        {
            std::lock_guard<std::mutex> lock(mutex);
            
            // Find an available connection or create a new one
            for (auto& c : connections) {
                if (!c->inUse) {
                    c->inUse = true;
                    conn = c;
                    break;
                }
            }
            
            if (!conn) {
                // Create a new connection if all are in use
                auto newConn = createConnection();
                if (newConn) {
                    conn = std::make_shared<DBConnection>(newConn);
                    conn->inUse = true;
                    connections.push_back(conn);
                }
            }
        }
        
        if (!conn) {
            Logger::error("Database health check failed: Unable to get connection");
            return false;
        }
        
        // Step 2: Perform a simple test query
        try {
            std::cout << "Health check: Executing test query..." << std::endl;
            
            // Create statement directly instead of using helper method
            std::unique_ptr<sql::Statement> stmt(conn->getConnection()->createStatement());
            if (!stmt) {
                std::cout << "Health check: Failed to create statement" << std::endl;
                throw std::runtime_error("Failed to create statement");
            }
            
            // Execute query and get result set
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1 AS test_value"));
            if (!res) {
                std::cout << "Health check: Failed to get result set" << std::endl;
                throw std::runtime_error("Failed to get result set");
            }
            
            // Check the result immediately
            bool hasRow = res->next();
            if (!hasRow) {
                std::cout << "Health check: Result set has no rows" << std::endl;
                throw std::runtime_error("Result set has no rows");
            }
            
            int value = res->getInt("test_value");
            std::cout << "Health check: Got value: " << value << std::endl;
            
            // Close the result set explicitly
            res.reset();
            
            // Close the statement explicitly
            stmt.reset();
            
            // Mark connection as not in use
            {
                std::lock_guard<std::mutex> lock(mutex);
                conn->inUse = false;
            }
            
            return (value == 1);
            
        } catch (const sql::SQLException& e) {
            std::cout << "Health check: SQL exception: " << e.what() << std::endl;
            Logger::error("Database health check SQL error: " + std::string(e.what()));
            
            // Make sure to release the connection even if there's an error
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (conn) conn->inUse = false;
            }
            
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Health check: General exception: " << e.what() << std::endl;
        Logger::error("Database health check failed: " + std::string(e.what()));
        
        // Make sure to release the connection even if there's an error
        if (conn) {
            std::lock_guard<std::mutex> lock(mutex);
            conn->inUse = false;
        }
        
        return false;
    }
}

std::shared_ptr<sql::Connection> DBConnectionPool::createConnection() {
    try {
        // Build connection properties
        // Build connection properties
        sql::SQLString url("jdbc:mariadb://" + host + ":" + std::to_string(port) + "/" + database);
        sql::Properties properties({
            {"user", user},
            {"password", password},
            {"autoReconnect", "true"},
            {"useUnicode", "true"},
            {"characterEncoding", "utf8mb4"},
            {"connectTimeout", "5000"},  // 5 second timeout
            {"socketTimeout", "5000"},   // 5 second timeout
            {"loginTimeout", "5000"}     // 5 second timeout
        });

        std::cout << "Attempting to connect to DB with URL: " << url.c_str() << std::endl;

        // Create connection
        sql::Connection* rawConn = driver->connect(url, properties);
        return std::shared_ptr<sql::Connection>(rawConn);
    }
    catch (const sql::SQLException& e) {
        std::stringstream ss;
        ss << "SQL Error creating database connection: " << e.what();
        Logger::error(ss.str());
        return nullptr;
    }
}



void DBConnectionPool::cleanup() {
    std::lock_guard<std::mutex> lock(mutex);

    // Clear all connections
    connections.clear();
    initialized = false;

    Logger::info("Database connection pool cleaned up");
}
