cmake_minimum_required(VERSION 3.10)
project(airlines_api_cpp)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)  # OpenSSL is required for jwt-cpp

# MariaDB Connector/C++ - Using direct path approach since find_package may not work
set(MARIADB_CONNECTOR_INCLUDE_DIR "/usr/include/mariadb" "/usr/include/mariadb/conncpp")
set(MARIADB_CONNECTOR_LIB "mariadbcpp")

# Include directories
include_directories(${MARIADB_CONNECTOR_INCLUDE_DIR})
include_directories(include)
# No need to explicitly include jwt-cpp as it's in the global include path

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(airline_api ${SOURCES})

# Link libraries
target_link_libraries(airline_api
    Threads::Threads
    ${MARIADB_CONNECTOR_LIB}
    nlohmann_json::nlohmann_json
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Copy configuration files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config.json DESTINATION ${CMAKE_BINARY_DIR})
