/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
*********/

// Import required libraries
#include "CaptureWiFi.h"
#include "custom_CaptureWiFiClass.h"
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>

CustomCaptureWiFiClass CaptureWiFi;

// Replace with your network credentials
const char* ssid = "TEST-DO-NOT-USE-PSK";
const char* password = "WiFiPassword";

#define DHTPIN 27     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// Create AsyncWebServer object on port 80
// AsyncWebServer server(80);

float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return 0.f;
  }
  else {
    Serial.println(t);
    // return String(t);
    return t;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    // return "--";
    return 0.f;
  }
  else {
    Serial.println(h);
    // return String(h);
    return h;
  }
}

// Replaces placeholder with DHT values
/* String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
  return String();
}*/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  dht.begin();

  // Connect to Wi-Fi
  CaptureWiFi.begin(ssid, password);
  while (CaptureWiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected.");

  // Print ESP32 Local IP Address
  // Serial.println(CaptureWiFi.localIP());

  // Route for root / web page
  /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  // Start server
  server.begin();*/
}

void loop(){

  if (CaptureWiFi.driver_connection.available()) {
    LOGV("New message from driver... \n");
    
    capture_thserver_driver_message_t message = capture_thserver_driver_message_t();
    CaptureWiFi.readFromTHServerDriver(&message);

    switch(message.type) {
      case thserver_driver_msg_type_t::GET_TEMP:
        Serial.println("Get Temperature");
        CaptureWiFi.writeToTHServerDriver(thserver_driver_msg_type_t::TEMP_VAL, readDHTTemperature());
        break;
      case thserver_driver_msg_type_t::GET_HUM:
        Serial.println("Get Humidity");
        CaptureWiFi.writeToTHServerDriver(thserver_driver_msg_type_t::HUM_VAL, readDHTHumidity());
        break;
      default:
        Serial.println("Unexpected message type!");
    }
  }
}
