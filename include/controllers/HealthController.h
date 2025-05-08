#pragma once

#include <crow.h>
#include <string>

class HealthController {
public:
    static crow::response checkHealth();
    static crow::response checkDatabaseHealth();
};
