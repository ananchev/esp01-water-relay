#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <helperfunctions.h>

// Wi-Fi and webserver settings 
const char* ssid = "CasaDiMi";
const char* password = "Muska1ova";
ESP8266WebServer server(80);

// Valid zone names to include in web server request
std::vector<String> validZones {
  "grass", 
  "flowers"
};

// Valid durations to run the water for
std::vector<String> validWateringDurations {
  "1hr",
  "2hrs",
  "8hrs",
  "12hrs" 
};

void handleIndex() {
  server.send(200, F("text/html"),
  F("Welcome to the REST Web Server"));
}

void handleArgs() {
  String message = "";

  if (!in_array(server.arg("zone"), validZones) ||
      !in_array(server.arg("duration"), validWateringDurations)
     )
  {
    message = "Please supply parameter 'zone' with value 'grass' or 'flowers'.";
  }
  else
  {     //Parameter found
    message = "zone = ";
    message += server.arg("zone");
    message += "duration = ";
    message += server.arg("duration");
  }

  server.send(200, "text/plain", message);          //Returns the HTTP response
}


 

// Define routing
void restServerRouting() {
  server.on("/", HTTP_GET, handleIndex);
  server.on(F("/water"), HTTP_GET, handleArgs);
  // server.on(F("/measurements"), HTTP_GET, serveAll);
  // server.on(F("/setInterval"), HTTP_GET, setInterval);
}


// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void setup() {
  Serial.begin(115200);
  WiFi.hostname("waterrelay");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handling of incoming requests
  server.handleClient();
}