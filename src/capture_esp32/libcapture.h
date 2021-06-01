#ifndef LIBCAPTURE_H
#define LIBCAPTURE_H

#include <WiFi.h> // ESP32 WiFi include

#define LOGV //Serial.printf
#define LOGE Serial.printf
#define SPECIAL_LOGV Serial.printf

int MAX_CAPTURE_MSG_SIZE = 4096;

class Capture
{
private:
    /* data */
    std::string monitor_addr;
    uint16_t monitor_port;
    WiFiClient wifi_client;
    uint16_t eap_driver_port;
    char *eap_username = nullptr;
    char *eap_password = nullptr;

public:
    Capture(/* args */);
    ~Capture();

    // network
    int connect_psk(const char *ssid, const char* password);
    int connect_eap();
    int network_read(uint8_t *dst, size_t len);
    int network_read(uint8_t *dst, size_t len, WiFiClient target);
    void network_write(const char *src, size_t len);
    void network_write(const char *src, size_t len, WiFiClient target);
    IPAddress driver_ip();
    uint16_t driver_port();

    // libcapture_crypto
    int generate_key_pair(const char *public_key_file, const char *private_key_file);

    // libcapture_monitor
    int discover_monitor();
    int connect_monitor();
    int receive_credentials(const char *driver);
};

#endif
