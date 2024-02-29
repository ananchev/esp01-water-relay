#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <helperfunctions.h>

// Wi-Fi and webserver settings
const char *ssid = "ssid";
const char *password = "password";
AsyncWebServer server(80);

// time variables
long prevReadingTime = 0;
long wateringInterval = 0;
long postpone_seconds = 0;
long remainingWateringTime = 0;

// Open the first relay： A0 01 01 A2
// Close the first Relay： A0 01 00 A1
// Open the second relay： A0 02 01 A3
// Close the second Relay：A0 02 00 A2

// the 2-way relay is managed via serial interface
const byte deactivate_relay_one[] = {0xA0, 0x01, 0x00, 0xA1};
const byte activate_relay_one[] = {0xA0, 0x01, 0x01, 0xA2};
const byte deactivate_relay_two[] = {0xA0, 0x02, 0x00, 0xA2};
const byte activate_relay_two[] = {0xA0, 0x02, 0x01, 0xA3};

// Store the relay states
int sprinklerRelay = 0;
int dripIrrigationRelay = 0;
String sprinklerRelayState;
String dripIrrigationRelayState;

// Stores the interval for which watering was set
String interval = "";

// Replaces placeholders with the relay states (called on calling the 'index' page)
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "SPRINKLER_STATE")
  {
    if (sprinklerRelay == 1)
    {
      sprinklerRelayState = "ON";
    }
    else
    {
      sprinklerRelayState = "OFF";
    }
    // Serial.print(relayState);
    return sprinklerRelayState;
  }
  else if (var == "DRIP_IRRIGATION_STATE")
  {
    if (dripIrrigationRelay == 1)
    {
      dripIrrigationRelayState = "ON";
    }
    else
    {
      dripIrrigationRelayState = "OFF";
    }
    // Serial.print(relayState);
    return dripIrrigationRelayState;
  }
  else
  {
    return "Nothing";
  }
};

// this is dummy callback function to test the workaround for delay with AsyncWebServer
void aR1(AsyncWebServerRequest *request)
{
  Serial.write(activate_relay_two, sizeof(activate_relay_two));

  auto seconds = 1;
  auto end = millis() + seconds * 1000;
  // this loop, when empty, causes a reboot after about 3 seconds
  // but imagine this is not an empty loop, but a real stuff that is just taking a bit of time…
  while (end > millis())
  {
    // yield(); // panic, crash, restart
    // ESP.wdtFeed(); // no crashes even for long times, but everything else is being blocked
  }

  Serial.write(activate_relay_one, sizeof(activate_relay_one));

  request->send(200, "text/plain", "OK now?");
}

void setup()
{

  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Store the remaining watering time
  remainingWateringTime = -1;

  // Initialise and connect Wi-Fi
  WiFi.hostname("waterrelay");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
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
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("'/' called");

    request->send(SPIFFS, "/index.html", String(), false, processor); });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  server.on("/start2", HTTP_GET, aR1);

  // Start the watering
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request)
            {
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

    // Determine the requested watering interval and start resp. water relay
    switch (resolveParameters(interval))
    {

    case OneHour:

      // turn off drip irrigation relay
      Serial.write(deactivate_relay_two, sizeof(deactivate_relay_two)); 
      // consecutive serial writes do not work for the relay module
      // cannot use yield or delay with ESPAsyncWebServer
      // https://github.com/me-no-dev/ESPAsyncWebServer#important-things-to-remember
      // workaround: https://github.com/me-no-dev/ESPAsyncWebServer/issues/1190
      postpone_seconds = millis() + 1 * 1000;
      while (postpone_seconds > millis()) {
        // do nothing for 1 second
      }
      
      // wateringInterval = 1*60*60*1000; // in milliseconds = 1 hour * 60 min * 60 sec * 1000
      wateringInterval = 1*1*60*1000; // in milliseconds = 1 hour * 60 min * 60 sec * 1000

      // turn on sprinkler relay
      Serial.write(activate_relay_one, sizeof(activate_relay_one));
      Serial.println("activate_relay_one");
      sprinklerRelay = 1;
      break;

    case EightHours:

      // turn off sprinkler relay
      Serial.write(deactivate_relay_one, sizeof(deactivate_relay_one)); 
      postpone_seconds = millis() + 1 * 1000;
      while (postpone_seconds > millis()) {
        // do nothing for 1 second
      }

      wateringInterval = 8*60*60*1000; // in milliseconds = 8 hours * 60 min * 60 sec * 1000

      // turn on drip irrgation relay
      Serial.write(activate_relay_two, sizeof(activate_relay_two));
      Serial.println("activate_relay_two");
      dripIrrigationRelay = 1;
      break;

    case TwelveHours:

      // turn off sprinkler relay
      Serial.write(deactivate_relay_one, sizeof(deactivate_relay_one)); 
      postpone_seconds = millis() + 1 * 1000;
      while (postpone_seconds > millis()) {
        // do nothing for 1 second
      }

      wateringInterval = 12*60*60*1000; // in milliseconds = 8 hours * 60 min * 60 sec * 1000

      // turn on drip irrgation relay
      Serial.write(activate_relay_two, sizeof(activate_relay_two));
      Serial.println("activate_relay_two");

      dripIrrigationRelay = 1;
      break;

    default:
      break;
    }

    prevReadingTime = millis();
    remainingWateringTime =  wateringInterval;

    request->send(SPIFFS, "/index.html", String(), false, processor); });

  server.on("/activeInterval", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("'/activeInterval' called");
    // return the last set interval
    request->send(200, "text/plain", interval.c_str()); });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("'/off' called");

    // reset interval
    remainingWateringTime = 0;
    prevReadingTime = 0;
    String lastSetInterval = interval; 
    interval = ""; 
    // return the last set interval so UI button gets deactivated
    request->send(200, "text/plain", lastSetInterval.c_str()); });

  server.on("/remainingWateringTime", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // Serial.println("'/remainingWateringTime' called");
    if (remainingWateringTime <= 0)
    {
      Serial.println("watering inactive");
    }
    else
    {
      // Serial.println("watering ongoing... ");
    }
    String retval = "{\"remainingSeconds\": " + String(remainingWateringTime/1000) + " ,\"intervalSet\": \"" + interval + "\"}";
    // add to the return value information which watering interval was set
    request->send(200, "text/plain", retval.c_str()); });

  // Start server
  server.begin();
}

void loop()
{
  // update the remaining time to keep water on
  if (remainingWateringTime > 0)
  {
    long now = millis();
    remainingWateringTime = remainingWateringTime - (now - prevReadingTime);
    prevReadingTime = now;
  }
  else if (remainingWateringTime == 0)
  {
    // turn off the water (both relays)
    Serial.write(deactivate_relay_one, sizeof(deactivate_relay_one));
    Serial.println("deactivate_relay_one");
    sprinklerRelay = 0;

    delay(1000);

    Serial.write(deactivate_relay_two, sizeof(deactivate_relay_two));
    Serial.println("deactivate_relay_two");
    dripIrrigationRelay = 0;
    Serial.println("Water was switched off as watering time completed");
    
    // reset the intervals
    remainingWateringTime = -1;
    prevReadingTime = 0;
    interval = "";
  }
  else
  {
    // do nothing as relay is not active
  }
}