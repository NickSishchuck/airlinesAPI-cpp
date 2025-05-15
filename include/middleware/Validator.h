#pragma once

#include <crow.h>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Define a validation rule
struct ValidationRule {
    std::string field;
    std::function<bool(const std::string&)> validator;
    std::string errorMessage;
};

class Validator {
public:
    // Flight validation
    static std::vector<ValidationRule> getFlightValidationRules();

    // Ticket validation
    static std::vector<ValidationRule> getTicketValidationRules();

    // User validation
    static std::vector<ValidationRule> getUserValidationRules();

    // Crew validation
    static std::vector<ValidationRule> getCrewValidationRules();

    // Crew member validation
    static std::vector<ValidationRule> getCrewMemberValidationRules();

    // Generic validation function
    static crow::response validate(const crow::request& req, const std::vector<ValidationRule>& rules);

    // Individual field validators
    static bool isValidEmail(const std::string& email);
    static bool isValidPassport(const std::string& passport);
    static bool isValidFlightNumber(const std::string& flightNumber);
    static bool isValidSeatNumber(const std::string& seatNumber);
    static bool isValidDate(const std::string& date);
    static bool isPositiveInteger(const std::string& num);
    static bool isPositiveFloat(const std::string& num);
    static bool isNotEmpty(const std::string& value);
};
