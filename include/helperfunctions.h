#include <Arduino.h>

bool in_array(const String value, const std::vector<String> validValues); // used in dev

enum WateringDuration {
    Option_Invalid,
    OneHour,
    EightHours,
    TwelveHours
};

WateringDuration resolveParameters(String input);
