#include <WiFi.h> // ESP32 WiFi include
#include "esp_wpa2.h"
#include "assert.h"

// #include "WiFiConfig.h" // PSK information

#include "libcapture.h"
using std::string;

/*
   Connect this device to PSK network to obtain internet access
*/
int Capture::connect_psk(const char *ssid, const char *password)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.print("Password:");
  Serial.println(password);

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);

    if ((++i % 16) == 0)
    {
      Serial.println(F(" still trying to connect"));
    }
  }

  Serial.print(F("Connected. My IP address is: "));
  Serial.println(WiFi.localIP());
  return 0;
}

/*
   Connect local device to the WPA-EAP network
*/
int Capture::connect_eap()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)eap_username, strlen(eap_username));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)eap_username, strlen(eap_username));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)eap_password, strlen(eap_password));
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
  esp_wifi_sta_wpa2_ent_enable(&config);

  string eap_ssid = "TEST-DO-NOT-USE";
  WiFi.begin(eap_ssid.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if ((++i % 60) == 0)
    {
      Serial.println(F(" still trying to connect"));
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return 0;
}

int Capture::network_read(uint8_t *dst, size_t len)
{
  return network_read(dst, len, wifi_client);
}

int Capture::network_read(uint8_t *dst, size_t len, WiFiClient target)
{
  assert(dst != nullptr);
  int num_read = -1;
  while (num_read < 0)
  {
    num_read = target.read((uint8_t *)dst, len);
  }
  return num_read;
}

void Capture::network_write(const char *src, size_t len)
{
  this->network_write(src, len, wifi_client);
}

void Capture::network_write(const char *src, size_t len, WiFiClient target)
{
  size_t sent = 0;
  while (sent < len)
  {
    size_t tmp = target.write(src + sent, len - sent);
    sent += tmp;

    // if (tmp == 0)
    // {
    //   break;
    // }
  }
}

IPAddress Capture::driver_ip()
{
  return WiFi.gatewayIP();
}

uint16_t Capture::driver_port()
{
  return eap_driver_port;
}