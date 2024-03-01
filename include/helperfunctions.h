#include <Arduino.h>

bool in_array(const String value, const std::vector<String> validValues); // used in dev

enum WateringDuration {
    Option_Invalid,
    HalfHour,
    OneHour,
    TwoHours
};

WateringDuration resolveParameters(String input);
