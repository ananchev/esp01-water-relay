#include <Arduino.h>
#include <helperfunctions.h>

WateringDuration resolveParameters(String input) {
    if( input == "1hr" ) return OneHour;
    if( input == "8hrs" ) return EightHours;
    if( input == "12hrs" ) return TwelveHours;  
    assert(false); //tell the compiler that the else case deliberately omitted
};