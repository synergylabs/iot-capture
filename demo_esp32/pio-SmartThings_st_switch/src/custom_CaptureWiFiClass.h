#ifndef CUSTOME_CAPTURE_WIFI_CLASS_H
#define CUSTOME_CAPTURE_WIFI_CLASS_H

#include "CaptureWiFiClass.h"
#include "capture_st_message.h"

class CustomCaptureWiFiClass : public CaptureWiFiClass
{
private:
public:
    using CaptureWiFiClass::CaptureWiFiClass;
    int readFromCameraDriver(capture_st_driver_message_t *read_data_msg);
    int readFromCameraDriver(capture_st_driver_message_t *read_data_msg, WiFiClient &connection);
    int writeToCameraDriver(st_driver_msg_type_t type, uint32_t topic_len,
                            const char *topic, uint32_t message_len, char *message);
    int writeToCameraDriver(st_driver_msg_type_t type, uint32_t topic_len,
                            const char *topic, uint32_t message_len, char *message,
                            WiFiClient &connection);

    int sendSubscribeMessage(const char *topic, uint32_t topic_len);
    int sendPublishMessage(const char *topic, uint32_t topic_len, char *message, uint32_t message_len);
};

extern CustomCaptureWiFiClass CaptureWiFi;

#endif
