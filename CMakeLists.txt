cmake_minimum_required(VERSION 3.10)
project(airlines_api_cpp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)

# MariaDB Connector/C++ - Using direct path approach since find_package may not work
set(MARIADB_CONNECTOR_INCLUDE_DIR "/usr/include/mariadb" "/usr/include/mariadb/conncpp")
set(MARIADB_CONNECTOR_LIB "mariadbcpp")

include_directories(${MARIADB_CONNECTOR_INCLUDE_DIR})
include_directories(include)

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
)

# Copy configuration files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/config.json DESTINATION ${CMAKE_BINARY_DIR})
