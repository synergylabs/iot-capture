#ifndef CAPTURE_CORE_CAPTURE_WIFI_SERVER_H
#define CAPTURE_CORE_CAPTURE_WIFI_SERVER_H

class CaptureWiFiServer
{
private:
    SemaphoreHandle_t server_mutex;
    bool is_started = false;

public:
    CaptureWiFiServer(uint16_t port = 80, uint8_t max_clients = 4);
    ~CaptureWiFiServer();

    void begin();
    CaptureWiFiClient* available();
};

#endif