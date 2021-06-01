# Apps for ESP32 platform

Requirement for WiFi connection: you need to create `WiFiConfig.h.default` with WPA2 PSK SSID and password. Example format for WiFi configuration file is listed in `connect_wifi/WiFiConfig.h.example`.

## Links

https://randomnerdtutorials.com/latching-power-switch-circuit-auto-power-off-circuit-esp32-esp8266-arduino/

## Device Connection Setup

ESP32 ---- Latching Relay
- 4 ---- SET
- 16 ---- UNSET
- GND ---- GND
- 3V3 ---- 3V
