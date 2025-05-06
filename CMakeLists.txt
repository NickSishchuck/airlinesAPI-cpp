cmake_minimum_required(VERSION 3.10)
project(airline_api_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include and source directories
include_directories(include)

# Find required packages
find_package(Threads REQUIRED)

# MySQL Connector/C++ (you might need to adjust paths based on your system)
set(MYSQL_CONNECTOR_DIR ${CMAKE_SOURCE_DIR}/lib/mysql-connector-cpp)
include_directories(${MYSQL_CONNECTOR_DIR}/include)
link_directories(${MYSQL_CONNECTOR_DIR}/lib)

# Crow is header-only, so we just need to include it
set(CROW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/crow/include)
include_directories(${CROW_INCLUDE_DIR})

# nlohmann/json is also header-only
set(JSON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/json/include)
include_directories(${JSON_INCLUDE_DIR})

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(airline_api ${SOURCES})

# Link libraries
target_link_libraries(airline_api
    Threads::Threads
    mysqlcppconn
)

# Copy configuration files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config.json DESTINATION ${CMAKE_BINARY_DIR})
