#include <Arduino.h>
#include <helperfunctions.h>

WateringDuration resolveParameters(String input) {
    if( input == "30min" ) return HalfHour;
    if( input == "1hr" ) return OneHour;
    if( input == "2hrs" ) return TwoHours;  
    assert(false); //tell the compiler that the else case deliberately omitted
};