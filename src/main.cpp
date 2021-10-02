#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <helperfunctions.h>



// Wi-Fi and webserver settings 
const char* ssid = "CasaDiMi";
const char* password = "Muska1ova";
AsyncWebServer server(80);



// 
long prevReadingTime = 0;
long wateringInterval = 0;
int remainingWateringTime;


// Set relay GPIO
const int relayPin = 0; // relay connected to  GPIO0

// Stores  state
String relayState;

// Stores the interval for which watering was set 
String interval = "";

// Replaces placeholder with LED state value
String processor(const String& var){
  // Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(relayPin)){
      relayState = "ON";
    }
    else{
      relayState = "OFF";
    }
    // Serial.print(relayState);
    return relayState;
  }  
  else {
    return "Nothing";
  }
};


void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS 
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
  return;
  }


  // Configure the relay pin as output and set it to Low
  pinMode(relayPin,OUTPUT);
  digitalWrite(relayPin, LOW);
  remainingWateringTime = -1;

  // Initialise and connect Wi-Fi
  WiFi.hostname("waterrelay");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print Wi-Fi connection info
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("'/' called");

    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });



  // Start the watering
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("'/start' called");

    //web request is as /start?interval=<intervalParameter>
    // check how many parameters were received
    int paramsNr = request->params(); 
    for(int i=0;i<paramsNr;i++){
        AsyncWebParameter* p = request->getParam(i);
        if (p->name() == "interval"){
          interval = p->value();
          break;
        }
        else{
          // if needed use for further logic to handle calls without interval value
        }
    }

    // Determine the watering interval
    switch (resolveParameters(interval))
    {
    case OneHour:
      wateringInterval = 1*60*60*1000; // in milliseconds = 1 hour * 60 min * 60 sec * 1000
      break;
    case EightHours:
      wateringInterval = 8*60*60*1000; // in milliseconds = 8 hours * 60 min * 60 sec * 1000
      break;
    case TwelveHours:
      wateringInterval = 12*60*60*1000; // in milliseconds = 8 hours * 60 min * 60 sec * 1000
      break;
    default:
      break;
    }

    digitalWrite(relayPin, HIGH); // turn on relay by set GPIO to HIGH

    prevReadingTime = millis();
    remainingWateringTime = prevReadingTime + wateringInterval;
    Serial.print("Water activated, will stop after ");
    Serial.print(remainingWateringTime);
    Serial.println();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  
  server.on("/activeInterval", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("'/activeInterval' called");
    // return the last set interval
    request->send(200, "text/plain", interval.c_str());
  });

  
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("'/off' called");

    digitalWrite(relayPin, LOW);
    remainingWateringTime = 0;
    prevReadingTime = 0;
    String lastSetInterval = interval; 
    interval = ""; 
    // return the last set interval so UI button gets deactivated
    request->send(200, "text/plain", lastSetInterval.c_str());
  });


  server.on("/remainingWateringTime", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("'/remainingWateringTime' called");
    if (remainingWateringTime <= 0)
    {
        Serial.println("watering inactive");
    }
    else
    {
        Serial.println("watering ongoing");
    }
    String retval = "{\"remainingSeconds\": " + String(remainingWateringTime/1000) + " ,\"intervalSet\": \"" + interval + "\"}";
    // add to the return value information which watering interval was set
    request->send(200, "text/plain", retval.c_str());
  });

  // Start server
  server.begin();

}

void loop() {
  // update the remaining time to keep water on
  if (remainingWateringTime > 0)
  {
      // Serial.println("remaining wtarting time >0"); 
      long now = millis();
      remainingWateringTime = remainingWateringTime - (now - prevReadingTime);
      prevReadingTime = now;
  }
  else if (remainingWateringTime == 0)
  { 
      // turn off the water
      digitalWrite(relayPin, LOW);
      Serial.println("relay was switched off as watering time completed");
      remainingWateringTime = -1;
  }
  else 
  {
      // do nothing as relay is not active
  }
  
    
}  


