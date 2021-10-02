#include <Arduino.h>
bool in_array(const String value, const std::vector<String> validValues)
{
    return std::find(validValues.begin(), validValues.end(), value) != validValues.end();
}