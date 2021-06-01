#include "CaptureWiFiServer.h"

CaptureWiFiServer::CaptureWiFiServer(uint16_t port, uint8_t max_clients)
{
    Serial.println("CaptureWiFiServer constructor...");
    Serial.printf("Port number: %u\n", port);
    is_started = false;
    server_mutex = xSemaphoreCreateMutex();
}

CaptureWiFiServer::~CaptureWiFiServer()
{
    Serial.println("CaptureWiFiServer destructor...");
}

void CaptureWiFiServer::begin()
{
    // TODO: create a webserver on the driver.
    lock_mutex(server_mutex);
    is_started = true;
    unlock_mutex(server_mutex);
}

CaptureWiFiClient* CaptureWiFiServer::available()
{
    if (!is_started) return nullptr;

    return CaptureWiFi.getNewClient();
}
