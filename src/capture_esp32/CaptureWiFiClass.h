#ifndef CAPTURE_CORE_CAPTURE_WIFI_CLASS_H
#define CAPTURE_CORE_CAPTURE_WIFI_CLASS_H

#include <WiFiClient.h>

#include "CaptureWiFiClient.h"
#include "libcapture.h"
#include "BufferedRw.h"

class CaptureWiFiClass
{
private:
    SemaphoreHandle_t mutex_wifi_class;
    int state;
    char *driver;

public:
    Capture capture;
    WiFiClient driver_connection;
    CaptureWiFiClass();
    CaptureWiFiClass(const char *driver_name);
    ~CaptureWiFiClass();

    void begin(const char *ssid, const char *password);
    int status();
    char *localIP();

    CaptureWiFiClient* getNewClient();
    uint8_t checkClientConnected(CaptureWiFiClient *client);
    uint8_t closeClientConnection(CaptureWiFiClient *client);
    
    int readFromDriver(CaptureWiFiClient *client, uint8_t *buf, size_t size);
    int writeToDriver(CaptureWiFiClient *client, const uint8_t *buf, size_t size);
};

#endif
